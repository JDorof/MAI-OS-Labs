#include <iostream>
#include <sys/wait.h>
#include <string>
#include <cstring>

// fd[0] - чтение
// fd[1] - запись

char * Prepare_str(std::string str) {
    return const_cast<char*>(str.c_str());
}

int main() {
    int fd[2];

    // Отлов ошибок при создании 
    if (pipe(fd) == -1) {
        std::cerr << "Error in creating pipe\n";
        return 1;
    }

    std::string out_file;
    std::cout << "Out file name: ";
    std::getline(std::cin, out_file);

    pid_t pid = fork();
    if (pid == -1) {
        std::cerr << "Error in creating fork()\n";
        return 1;
    }

    if (pid == 0) { // Дочерний процесс
        
        close(fd[1]);  // Закрываем сторону записи в pipe, так как дочерний процесс будет только читать

        // Аргументы для execve
        char* args[] = {"./child", Prepare_str(std::to_string(fd[0])), Prepare_str(out_file), NULL};

        // Выполняем дочернюю программу
        execve("./child", args, NULL);
        std::cerr << "error in execve\n";
        return 1;
    } else { // Родительский процесс
        close(fd[0]); // Закрытие дескриптора чтения, так как идет запись

        // Ввод чисел
        std::string input;
        std::cout << "Divider and dividers: ";
        std::getline(std::cin, input);

        write(fd[1], input.c_str(), input.size());
        close(fd[1]);

        // Ожидаем завершения дочернего процесса
        wait(NULL);
    }

    return 0;
}