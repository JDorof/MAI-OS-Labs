#!/bin/bash

g++ main.cpp -o main

# Функция, чтобы замерять время исполнения программы для разного числа потоков
function get_exec_time {
    local threads=$1
    local start_time=$(date +%s%3N)
    ./main 3 $threads in.txt out.txt 20
    local end_time=$(date +%s%3N)
    echo $((end_time - start_time))
}

# Замеряем время для одного потока
T1=$(get_exec_time 1)
echo "Количество потоков: 1, Время исполнения: $T1"

for p in {2..8}; do
    Tp=$(get_exec_time $p)
    Sp=$(echo "scale=2; $T1 / $Tp" | bc) # Ускорение
    Xp=$(echo "scale=2; $Sp / $p" | bc)  # Эффективность
    echo "Количество потоков: $p, Ускорение: $Sp, Эффективность: $Xp, Время исполнения: $Tp"
done
