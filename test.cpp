#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

std::string currentTimeAsString() {
    // Получаем текущее время
    auto now = std::chrono::system_clock::now();
    // Преобразуем время в time_t
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    // Преобразуем time_t в структуру tm
    std::tm *now_tm = std::localtime(&now_time);

    // Используем std::ostringstream для форматирования времени
    std::ostringstream oss;
    oss << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

int main() {
    std::string timeString = currentTimeAsString();
    std::cout << "Current time: " << timeString << std::endl;
    return 0;
}
