#include "Client.hpp"

int main() {
    std::cout << "START CLIENT NODE" << std::endl;
    Client server("5555");
    server.run();
}