#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <time.h>


int** read_file(const char* file_name, int& rows, int& columns) {
    FILE *in=fopen(file_name, "rt");
    fscanf(in,"%d %d", &rows, &columns);
    int** matrix = new int*[rows];
    for(int i = 0; i < rows; ++i) {
        matrix[i] = new int[columns];
        for(int j = 0; j < columns; ++j) {
            fscanf(in, "%d", &matrix[i][j]);
        }
    }
    fclose(in);
    return matrix;
}

void write_to_file(const char* file_name, const int& rows, const int& columns, int** matrix) { 
    FILE *out=fopen(file_name, "wt");
    fprintf(out, "%d %d\n", rows, columns);
    for(int i = 0; i < rows; ++i) {
        for(int j = 0; j < columns; ++j) {
            fprintf(out,"%d ", matrix[i][j]);
        }
        fprintf(out, "\n");
    }
    fclose(out);
}

int middle(int** matrix, const int& mi, const int& mj, int window, const int& rows, const int& columns) {
    int window_list[window * window];
    int c = 0;
    int fi, fj;
    for (int i = mi - (window / 2); i <= mi + (window / 2); ++i) {
        if (i >= 0) fi = i % rows;
        else fi = rows + i;
        std::cout << "fi = " << fi;
        for (int j = mj - (window / 2); j <= mj + (window / 2); ++j) {
            if (j >= 0) fj = j % columns;
            else fj = columns + j;
            window_list[c] = matrix[fi][fj];
            ++c;
            std::cout << "\tfj = " << fj << "\tvalue = " << matrix[fi][fj] << std::endl;
        }
    }
    std::sort(window_list, window_list + c);
    std::cout << "window_list = ";
    for (int c_ = 0; c_ < c; ++c_)
    std::cout << window_list[c_] << ' ';
    std::cout << "\nmiddle value = " << window_list[c/2] << "\tc = " << c << std::endl;
    return window_list[c/2];
}

int main(int argc, char *argv[]) {

    // setlocale(LC_ALL, "ru");

    if (argc < 5) {
        std::cout << "Использование: " << argv[0] << " <window_size> <num_threads> <in_file> <out_file> <k>\n";
        return 1;
    }

    time_t start = clock();

    int rows;
    int columns;
    int MAX_THREADS = 20;

    // Чтение параметров
    int window_size = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    const char* in_file = argv[3];
    const char* out_file = argv[4];

    if (num_threads > MAX_THREADS) {
        printf("Максимальное количество потоков: %d\n", MAX_THREADS);
        return 1;
    }

    // int window = 5;
    int** matrix = read_file(in_file, rows, columns);
    if (!matrix) { // Проверка на случай неудачного чтения
        std::cerr << "Ошибка при чтении файла " << in_file << std::endl;
        return 1;
    }

    int** result = new int*[rows];
    for (int i = 0; i < rows; ++i)
    result[i] = new int[columns];

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < columns; ++j) {
            std::cout << "i = " << i << "\tj = " << j << std::endl;
            result[i][j] = middle(matrix, i, j, window_size, rows, columns);
        }
    }

    // Окончание работы
    write_to_file(out_file, rows, columns, result);

    for (int i = 0; i < rows; ++i) {
        delete[] result[i];
        delete[] matrix[i];
    }
    delete[] result;
    delete[] matrix;

    std::cout << "execution time: " << clock() - start << std::endl; 
}