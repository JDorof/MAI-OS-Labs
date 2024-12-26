#pragma once

#include <chrono>
#include <future>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <unistd.h> // Include this header for fork and execl
#include <vector>
#include <zmq.hpp>
#include <unordered_map> // Include this header for std::unordered_map
#include <sstream> // Include this header for std::istringstream
#include <csignal> // Include this header for signal handling
#include <fstream>

#include "NTree.hpp"
#include "SocketCommunication.hpp"
#include "Utils.hpp"

class Client {
private:
    NTree topology;
    int counter;
    bool running;

    // Для общения с корневым вычислительным узлом
    zmq::context_t context;
    std::string port;
    zmq::socket_t socket;

    // Для асинхронной обработки
    std::unordered_map<int, std::promise<std::string>> request_promise_map;
    std::vector<std::future<void>> futures;

    std::string request_helper(const std::string &id, const std::string &cmd, const std::vector<std::string> &params) {
        try {
            if (!topology.find(std::stoi(id))) {
                return "Error: Not found";
            }
        } catch (const std::invalid_argument& e) {
            return "Error: Invalid ID";
        }

        int req_id = ++counter;
        std::string path = topology.get_path_to(std::stoi(id));

        std::vector<std::string> all_params = {std::to_string(req_id), cmd, path, id};
        all_params.insert(all_params.end(), params.begin(), params.end());
        std::string message = join(all_params, ";");

        std::cout << "[Client::request_helper] Setting promises" << std::endl;
        std::promise<std::string> promise;
        std::future<std::string> future = promise.get_future();
        request_promise_map[req_id] = std::move(promise);

        std::cout << "[Client::request_helper] Send message" << std::endl;
        if (!Send(socket, message)) {
            return "Error: Failed " + cmd;
        }

        std::cout << "[Client::request_helper] Wait to answer" << std::endl;
        return future.get();
    }

    std::string ping(const std::string &id) {
        return request_helper(id, "ping", {});
    }

    std::string pingall() {
        std::vector<std::string> unavailable_id;
        for (int id : topology.get_ids()) {
            std::string sid = std::to_string(id);
            std::string ping_response = ping(sid);
            if (ping_response != "Ok: 1") {
                unavailable_id.push_back(sid);
            }
        }

        if (unavailable_id.size() == 0) {
            return "Ok: -1";
        } else {
            return "Ok: " + join(unavailable_id, ";");
        }
    }

    std::string bind(std::string parent_id, const std::string &bind_port) {
        std::string ping_response = ping(parent_id);
        if (ping_response != "Ok: 1") {
            return "Error: Node is unavailable";
        }

        return request_helper(parent_id, "bind", {bind_port});
    }

    std::string create(const std::string &parent_id, const std::string &id) {
        // Проверяем, существует ли узел с таким айди
        int parent_id_int = std::stoi(parent_id);
        int iid = std::stoi(id);
        if (topology.find(iid)) {
            return "Error: Already exists";
        }

        // Добавляем узел в топологию
        topology.add(parent_id_int, iid);
        std::string parent_port;

        // Выбираем порт для подключения нового узла к родителю
        if (parent_id_int == -1) {
            parent_port = port;
        } else {
            parent_port = get_free_port();
        }

        // Пингуем родителя, чтобы убедиться, что он существует
        if (parent_id_int != -1) {
            std::string ping_response = ping(parent_id);
            if (ping_response == "Ok: 0") {
                return "Error: Parent is unavailable";
            }
            if (ping_response == "Error: Not found") {
                return "Error: Parent not found"; // По сути, эта ошибка никогда не будет выведена
            }
        }

        // Отправляем родителю сообщение, чтобы он забиндил свой сокет на нужный порт
        if (parent_port != port) {
            std::string bind_response = bind(parent_id, parent_port);

            if (bind_response == "Error") {
                return "Error: Parent cant bind socket to port " + parent_port;
            }
        }

        // Создаём процесс вычислительного узла
        pid_t pid = fork();
        if (pid == -1) {
            return "Error: Fork failed";
        }
        if (pid == 0) {
            // Дочерний процесс
            execl("./src/server", id.c_str(), parent_port.c_str(), NULL);
            // Если execl вернется, значит произошла ошибка
            std::cerr << "Error: execl failed" << std::endl;
            _exit(EXIT_FAILURE); // Завершаем дочерний процесс
        } else {
            // Родительский процесс
            std::ofstream file("log.txt", std::ios::app);
            if (file.is_open()) {
                file << pid << '\n';
                file.close();
            } else {
                std::cerr << "Unable to open file";
            }
            return "Ok: " + std::to_string(pid);
        }
    }

    std::string exec(const std::string &id, const std::string &args) {
        return request_helper(id, "exec", {args});
    }

    void process_user_command(const std::string &command_message) {
        std::string cmd;
        std::string id;
        std::istringstream iss(command_message);
        iss >> cmd;
        if (cmd == "exit") {
            running = false;
            return;
        }
        if (cmd == "pingall") {
            futures.push_back(std::async(std::launch::async, [this](){ std::cout << pingall() << std::endl; }));
        }
        iss >> id;
        if (!is_int(id)) {
            std::cerr << "[Client::process_user_command] Invalid id: " << id << std::endl;
            return;
        }

        std::cout << "[Client::process_user_command] Received user command " << cmd << std::endl;
        if (cmd == "create") {
            std::string parent_id = id;
            id.clear();
            iss >> id;

            if (!is_int(id)) {
                std::cerr << "[Client::process_user_command] Invalid id: " << id << std::endl;
                return;
            }

            futures.push_back(std::async(std::launch::async, [this, parent_id, id]() { std::cout << create(parent_id, id) << std::endl; }));
        } else if (cmd == "ping") {
            std::cout << "[Client::process_user_command] Start async function" << std::endl;
            // futures.push_back(std::async(std::launch::async, [this, id]() { std::cout << ping(id) << std::endl; }));
            futures.push_back(std::async(std::launch::async, [this, id](){ std::cout << ping(id) << std::endl; }));
        } else if (cmd == "exec") {
            std::string args;
            iss >> args;
            futures.push_back(std::async(std::launch::async, [this, id, args]() { std::cout << exec(id, args) << std::endl; }));
        } else {
            std::cout << "Unknown command" << std::endl;
        }
    }

    // Обработка сообщений от системы
    void process_incoming_message(const std::string &message) {
        std::vector<std::string> tokens = split(message, ';');
        int request_id = std::stoi(tokens[0]);
        std::string answer = tokens[1];

        if (request_promise_map.count(request_id)) {
            request_promise_map[request_id].set_value(answer);
            request_promise_map.erase(request_id);
        }
    }

public:
    Client(const std::string &port)
    : topology()
    , counter(0)
    , context(1)
    , port(port)
    , socket(context, ZMQ_PAIR)
    , request_promise_map()
    , futures()
    , running(true) {
        socket.bind("tcp://*:" + port);

        // Устанавливаем параметры сокета
        socket.set(zmq::sockopt::linger, 2000); // Удаление сообщений, если отправить нельзя
        socket.set(zmq::sockopt::rcvtimeo, 2000); // Тайм-аут на прием
        socket.set(zmq::sockopt::sndtimeo, 2000); // Тайм-аут на отправку
    }

    void run() {
        zmq::pollitem_t items[] = {{socket, 0, ZMQ_POLLIN, 0}};

        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            // Опрашиваем сокет на наличие сообщений
            int events = zmq::poll(items, 1, std::chrono::milliseconds(0)); // Немедленное возвращение
            if (events < 0) {
                std::cerr << "[Client::run] zmq::poll failed" << std::endl;
                continue;
            }

            if (items[0].revents & ZMQ_POLLIN) {
                std::optional<std::string> message = Receive(socket);
                if (message.has_value()) {
                    process_incoming_message(message.value());
                } else {
                    std::cerr << "[Client::run] Failed to receive message" << std::endl;
                }
            }
            // Обработка пользовательских команд
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(0, &readfds);
            struct timeval timeout = {0, 0}; // Немедленное возвращение
            if (select(1, &readfds, nullptr, nullptr, &timeout) > 0) {
                std::string request;
                std::getline(std::cin, request);

                std::cout << "[Client::run] Processing user request: " << request << std::endl;
                process_user_command(request);
            }
        }

        for (auto &future : futures) {
            future.get();
        }
    }



    ~Client() {
        std::cout << "Destroying System..." << std::endl;

        try {
            if (socket.handle() != nullptr) {
                Send(socket, "recursive_destroy");
                socket.close(); // Закрытие сокета
            }
        } catch (const zmq::error_t& e) {
            std::cerr << "Error while destroying Client: " << e.what() << std::endl;
        }

        context.close(); // Закрытие контекста
        std::cout << "System destroyed." << std::endl;
    }
};
