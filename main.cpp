#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <pthread.h>
#include <time.h>

pthread_mutex_t console_mutex;

int** read_file(const char* file_name, int& rows, int& columns) {
    FILE *in = fopen(file_name, "rt");
    fscanf(in, "%d %d", &rows, &columns);
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
    FILE *out = fopen(file_name, "wt");
    fprintf(out, "%d %d\n", rows, columns);
    for(int i = 0; i < rows; ++i) {
        for(int j = 0; j < columns; ++j) {
            fprintf(out, "%d ", matrix[i][j]);
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
        for (int j = mj - (window / 2); j <= mj + (window / 2); ++j) {
            if (j >= 0) fj = j % columns;
            else fj = columns + j;
            window_list[c] = matrix[fi][fj];
            ++c;
        }
    }
    std::sort(window_list, window_list + c);
    return window_list[c / 2];
}

// Struct to pass data to each thread
struct ThreadData {
    int** matrix;
    int** result;
    int window_size;
    int rows;
    int columns;
    int start_index;
    int end_index;
};

// Thread function for processing a segment of the matrix
void* process_segment(void* arg) {
    pthread_mutex_lock(&console_mutex);
    std::cout << "thread ID: " << pthread_self() << std::endl;
    pthread_mutex_unlock(&console_mutex);
    
    ThreadData* data = (ThreadData*)arg;
    for (int idx = data->start_index; idx < data->end_index; ++idx) {
        int i = idx / data->columns; // вычисляем номер строки
        int j = idx % data->columns; // вычисляем номер столбца
        data->result[i][j] = middle(data->matrix, i, j, data->window_size, data->rows, data->columns);
    }
    return nullptr;
}

int main(int argc, char *argv[]) {
    if (argc < 6) {
        std::cout << "Использование: " << argv[0] << " <window_size> <num_threads> <in_file> <out_file> <num_iterations>\n";
        return 1;
    }

    time_t start = clock();

    int rows, columns;
    int MAX_THREADS = 20;

    int window_size = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    const char* in_file = argv[3];
    const char* out_file = argv[4];
    int num_iterations = atoi(argv[5]);  // Количество итераций применения фильтра

    if (num_threads > MAX_THREADS) {
        std::cout << "Максимальное количество потоков: " << MAX_THREADS << std::endl;
        return 1;
    }

    int** matrix = read_file(in_file, rows, columns);
    if (!matrix) {
        std::cerr << "Ошибка при чтении файла " << in_file << std::endl;
        return 1;
    }

    int** result = new int*[rows];
    for (int i = 0; i < rows; ++i) {
        result[i] = new int[columns];
    }

    for (int iteration = 0; iteration < num_iterations; ++iteration) {
        std::cout << "\tNumber of iterations = " << iteration << std::endl;
        pthread_t threads[MAX_THREADS];
        ThreadData thread_data[MAX_THREADS];
        int total_elements = rows * columns;
        int elements_per_thread = total_elements / num_threads;
        int extra_elements = total_elements % num_threads;

        int start_index = 0;
        for (int t = 0; t < num_threads; ++t) {
            int end_index = (t == num_threads - 1) ? total_elements : start_index + elements_per_thread + (t < extra_elements ? 1 : 0);
            thread_data[t] = {matrix, result, window_size, rows, columns, start_index, end_index};
            pthread_create(&threads[t], nullptr, process_segment, &thread_data[t]);
            start_index = end_index;
        }

        for (int t = 0; t < num_threads; ++t) {
            pthread_join(threads[t], nullptr);
        }

        // Обновляем исходную матрицу результатом текущей итерации
        std::swap(matrix, result);
    }

    write_to_file(out_file, rows, columns, matrix);

    for (int i = 0; i < rows; ++i) {
        delete[] result[i];
        delete[] matrix[i];
    }
    delete[] result;
    delete[] matrix;

    std::cout << "\texecution time: " << (clock() - start) / (double)CLOCKS_PER_SEC << " seconds" << std::endl;
    return 0;
}
