#include "TimeNode.hpp"

int main(int argc, char* argv[]) {
    std::cout << "START SERVER NODE" << std::endl;
    // Разбираем параметры
    int id = atoi(argv[0]);
    std::string parent_port(argv[1]);

    // Создаём объект вычислительного узла
    TimeNode node(id, parent_port);

    // Запускаем его
    node.run();
}