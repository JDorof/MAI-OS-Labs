#include <iostream>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: [%s] [<read pipe>] [<output_file>]\n", argv[0]);
        return 1;
    }

    // Получаем дескриптор pipe для чтения
    int read_pipe = atoi(argv[1]);

    // Чтение данных из pipe
    char buffer[256];
    read(read_pipe, buffer, sizeof(buffer));
    close(read_pipe);

    // делим первое на два следующих
    char* token = strtok(buffer, " ");
    float result = atof(token);
    token = strtok(NULL, " ");
    while (token != NULL) {
        if (atof(token) != 0.0) {
            result /= atof(token);
            token = strtok(NULL, " ");
        } else {
            std::cerr << "Dividing by zero\n";
            return 1;
        }
    }

    // Открываем файл для записи результата
    int file = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file == -1) {
        std::cerr << "open file error\n";
        return 1;
    }

    // Записываем результат в файл
    dprintf(file, "%f", result);
    close(file);

    return 0;
}
