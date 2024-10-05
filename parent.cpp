#include <iostream>
#include <sys/wait.h>
#include <cstring>


int main() {
    // fd для передачи данных от родителя к дочернему процессу
    int fd[2];
    if (pipe(fd) == -1) {
        perror("Error in creating pipe");
        return 1;
    }

    std::string out_file;
    std::cout << "Out file name: ";
    std::getline(std::cin, out_file);

    pid_t pid = fork();
    if (pid == -1) {
        perror("Ошибка при fork()");
        return 1;
    }

    if (pid == 0) {
        // Дочерний процесс
        close(fd[1]);  // Закрываем сторону записи в pipe

        // Подготовка аргументов для execve
        char pipe_read_fd[10], file_name_arg[256];
        sprintf(pipe_read_fd, "%d", fd[0]);
        strcpy(file_name_arg, output_file.c_str());

        // Аргументы для execve
        char *args[] = {"./child", pipe_read_fd, file_name_arg, NULL};

        // Выполняем дочернюю программу
        execve("./child", args, NULL);
        perror("execve");
        exit(1);
    } else {
        // Родительский процесс
        close(fd[0]);

        // Ввод чисел
        std::string input_data;
        std::cout << "Введите числа через пробел: ";
        std::getline(std::cin, input_data);

        write(fd[1], input_data.c_str(), input_data.size());
        close(fd[1]);

        // Ожидаем завершения дочернего процесса
        wait(NULL);
    }

    return 0;
}