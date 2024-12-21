#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Использование: %s <file_path> <int FILESIZE>\n", argv[0]);
        return 1;
    }

    char* filepath = argv[1];
    int FILESIZE = atoi(argv[2]);

    // Открываем файл и отображаем его в память
    int fd = open(filepath, O_RDWR);
    if (fd == -1) {
        perror("Error opening file in child.");
        return 1;
    }

    char* mapped = (char*)mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("Error mapping file in child.");
        close(fd);
        return 1;
    }
    close(fd);

    // делим первое на два следующих
    char* token = strtok(mapped, " ");
    float result = atof(token);
    token = strtok(NULL, " ");
    while (token != NULL) {
        if (atof(token) != 0.0) {
            result /= atof(token);
            token = strtok(NULL, " ");
        } else {
            perror("Dividing by zero\n");
            return 1;
        }
    }

    printf("Result: %f\n", result);

    // Освобождаем память
    munmap(mapped, FILESIZE);

    return 0;
}
