#pragma once

#include <chrono>
#include <future>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <zmq.hpp>

#include "SocketCommunication.hpp"
#include "Utils.hpp"

class TimeNode {
public:
    int id;

    zmq::context_t context;
    zmq::socket_t parent_socket;
    std::vector<zmq::socket_t> kids_sockets;

    zmq::pollitem_t ps;
    std::vector<zmq::pollitem_t> ks;

    bool running;

    std::chrono::_V2::system_clock::time_point start;
    std::chrono::_V2::system_clock::time_point stop;
    bool is_stopped;

    void process_message(const std::string &message) {
        if (message == "recursive_destroy") {
            running = false;
            for (auto &kid : kids_sockets) {
                Send(kid, "recursive_destroy");
            }
            return;
        }
        std::vector<std::string> tokens = split(message, ';');
        std::string request_id = tokens[0];
        std::string cmd = tokens[1];
        std::string path = tokens[2];
        std::string id = tokens[3];

        std::string unavailable_message;
        if (cmd == "ping") {
            unavailable_message = "Ok: 0";
        } else if (cmd == "exec") {
            unavailable_message = "Error:" + id + ": Node is unavailable";
        } else if (cmd == "bind") {
            unavailable_message = "Error";
        }

        if (path.size() != 0) {
            tokens[2] = path.substr(1, path.size() - 1);
            if (static_cast<int>(path[0]) >= kids_sockets.size() or
                !Send(kids_sockets[static_cast<int>(path[0])], join(tokens, ";"))) {
                Send(parent_socket, join({request_id, unavailable_message}, ";"));
            }
            return;
        }

        if (cmd == "ping") {
            Send(parent_socket, join({request_id, "Ok: 1"}, ";"));
        } else if (cmd == "bind") {
            std::string port = tokens[4];
            setup_kid_socket(port);
            Send(parent_socket, join({request_id, "Ok: Binded"}, ";"));
        } else if (cmd == "exec") {
            std::string subcmd = tokens[4];
            if (subcmd == "start") {
                start = std::chrono::system_clock::now();
                is_stopped = false;
                Send(parent_socket, join({request_id, "Timer started"}, ";"));
            } else if (subcmd == "time") {
                double seconds;
                if (is_stopped) {
                    auto duration_start = start.time_since_epoch();
                    auto duration_stop = stop.time_since_epoch();
                    seconds = std::chrono::duration<double>(duration_stop).count() -
                              std::chrono::duration<double>(duration_start).count();
                } else {
                    stop = std::chrono::system_clock::now();
                    auto duration_start = start.time_since_epoch();
                    auto duration_stop = stop.time_since_epoch();
                    seconds = std::chrono::duration<double>(duration_stop).count() -
                              std::chrono::duration<double>(duration_start).count();
                }
                std::string message = std::to_string(seconds);
                Send(parent_socket, join({request_id, message}, ";"));
            } else if (subcmd == "stop") {
                is_stopped = true;
                stop = std::chrono::system_clock::now();
                Send(parent_socket, join({request_id, "Timer stopped"}, ";"));
            }
        } else {
            Send(parent_socket, join({request_id, "Error: Unknown command"}, ";"));
        }
    }

    void setup_kid_socket(const std::string &port) {
        kids_sockets.emplace_back(context, ZMQ_PAIR);
        kids_sockets.back().bind("tcp://*:" + port);
        kids_sockets.back().set(zmq::sockopt::linger, 2000);
        kids_sockets.back().set(zmq::sockopt::rcvtimeo, 2000);
        kids_sockets.back().set(zmq::sockopt::sndtimeo, 2000);

        zmq::pollitem_t item;
        item.socket = static_cast<void *>(kids_sockets.back());
        item.fd = 0;
        item.events = ZMQ_POLLIN;
        item.revents = 0;

        ks.push_back(item);

        std::cout << "Child socket bound to tcp://localhost:" << port << std::endl;
    }

public:
    TimeNode(int node_id, const std::string &port)
    : id(node_id)
    , context(1)
    , parent_socket(context, ZMQ_PAIR)
    , running(true)
    , start(std::chrono::system_clock::now())
    , stop(std::chrono::system_clock::now())
    , is_stopped(true) {
        parent_socket.connect("tcp://localhost:" + port);

        // Устанавливаем параметры сокета
        parent_socket.set(zmq::sockopt::linger, 2000); // Удаление сообщений, если отправить нельзя
        parent_socket.set(zmq::sockopt::rcvtimeo, 2000); // Тайм-аут на прием
        parent_socket.set(zmq::sockopt::sndtimeo, 2000); // Тайм-аут на отправку

        ps = zmq::pollitem_t{parent_socket, 0, ZMQ_POLLIN, 0};
        std::cout << "Parent socket connected to tcp://*:" << port << std::endl;
    }

    void run() {
        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            // Обработка сообщений от родителя
            zmq::pollitem_t items[] = {ps};
            zmq::poll(items, 1, std::chrono::milliseconds(0));

            if (items[0].revents & ZMQ_POLLIN) {
                std::optional<std::string> message = Receive(parent_socket);
                if (message.has_value()) {
                    std::cout << "[TimeNode::run] Received message from parent: " << message.value() << std::endl;
                    process_message(message.value());
                }
            }

            // Обработка сообщений от детей
            for (int kid_id = 0; kid_id < kids_sockets.size(); ++kid_id) {
                zmq::pollitem_t items[] = {ks[kid_id]};
                zmq::poll(items, 1, std::chrono::milliseconds(0));
                if (items[0].revents & ZMQ_POLLIN) {
                    std::optional<std::string> message = Receive(kids_sockets[kid_id]);
                    if (message.has_value()) {
                        std::cout << "[TimeNode::run] Received message from kid: " << message.value() << std::endl;
                        Send(parent_socket, message.value());
                    }
                }
            }
        }
    }

    ~TimeNode() {
        std::cout << "Destroying Node " << id << std::endl;

        try {
            for (auto &kid_socket : kids_sockets) {
                if (kid_socket.handle() != nullptr) {
                    Send(kid_socket, "recursive_destrimeNode");
                    kid_socket.close();
                }
            }
            if (parent_socket.handle() != nullptr) {
                parent_socket.close(); // Закрытие родительского сокета
            }
        } catch (const zmq::error_t &e) {
            std::cerr << "Error while destroying TimeNode: " << e.what() << std::endl;
        }

        context.close(); // Закрытие контекста
        std::cout << "Node " << id << " destroyed." << std::endl;
    }
};