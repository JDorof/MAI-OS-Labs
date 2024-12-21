#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#define FILEPATH "tmp/file"
// #define FILEPATH "/mnt/c/Users/JustDema/Desktop/University/2 year/MAI-OS-Labs/tmp/file"
#define FILESIZE 4096

pid_t child_pid;
void signal_handler(int sig) {}

int main() {
    int fd;
    char* mapped;

    fd = open(FILEPATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        perror("Error opening file.");
        exit(1);
    }
    if (ftruncate(fd, FILESIZE) == -1) {
        perror("Error setting file size.");
        close(fd);
        exit(1);
    }

    // Отображаем файл
    mapped = (char*)mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        perror("Error mapping file.");
        close(fd);
        exit(1);
    }
    close(fd);

    child_pid = fork();
    if (child_pid == -1) {
        perror("Fork failed.");
        munmap(mapped, FILESIZE);
        exit(1);
    } else if (child_pid == 0) {
        // Дочерний процесс: установка обработчика для SIGUSR1
        signal(SIGUSR1, signal_handler);
        pause();

        // execve доч проц
        char filesize_str[10];
        sprintf(filesize_str, "%d", FILESIZE);
        const char* args[] = {"./child", FILEPATH, filesize_str, NULL};

        execve("./child", (char* const*)args, NULL); 
        perror("execve failed.");
        exit(1);
    } else {
        // род проц

        char input_data[FILESIZE];
        printf("Введите числа через пробел: ");
        fgets(input_data, sizeof(input_data), stdin);

        // копирование в фалй
        memcpy(mapped, input_data, FILESIZE);
        // сигнал SIGUSR1 дочернему процессу
        kill(child_pid, SIGUSR1);

        wait(NULL);
        munmap(mapped, FILESIZE);
    }

    return 0;
}