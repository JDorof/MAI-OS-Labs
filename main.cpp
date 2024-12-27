#include <cstdlib>
#include <iostream>
#include <exception>
#include <chrono>

#include "MCKAllocator.hpp"
#include "PowerOfTwoAllocator.hpp"

void printDuration(const std::string& action, const std::chrono::high_resolution_clock::time_point& start) {
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << action << " took " << duration.count() << " microseconds.\n";
}

void testMCKAllocator() {
    const size_t memorySize = 1024; // Общий объем памяти (в байтах)
    const size_t blockSize = 256;   // Размер блока (по умолчанию)

    auto start = std::chrono::high_resolution_clock::now();
    void* memory = std::malloc(memorySize);
    printDuration("Memory allocation for MCK Allocator", start);
    if (!memory) {
        std::cerr << "Failed to allocate memory for MCK Allocator.\n";
        return;
    }

    try {
        start = std::chrono::high_resolution_clock::now();
        MCKAllocator allocator(memory, memorySize, blockSize);
        printDuration("MCKAllocator creation", start);

        start = std::chrono::high_resolution_clock::now();
        void* block1 = allocator.alloc();
        void* block2 = allocator.alloc();
        void* block3 = allocator.alloc();
        printDuration("Allocating blocks in MCK Allocator", start);

        allocator.debugPrint();

        start = std::chrono::high_resolution_clock::now();
        allocator.free(block1);
        allocator.free(block2);
        allocator.free(block3);
        printDuration("Freeing blocks in MCK Allocator", start);

        allocator.debugPrint();

        std::free(memory);
    } catch (const std::exception& e) {
        std::cerr << "MCK Allocator Error: " << e.what() << "\n";
        std::free(memory);
    }
}

void testPowerOfTwoAllocator() {
    const size_t memorySize = 1024;  // Общий объем памяти (в байтах)
    const size_t minBlockSize = 16;  // Минимальный размер блока (в байтах)
    const size_t maxBlockSize = 128; // Максимальный размер блока (в байтах)

    // Выделяем память для аллокатора
    auto start = std::chrono::high_resolution_clock::now();
    void* memory = std::malloc(memorySize);
    printDuration("Memory allocation for PowerOfTwo Allocator", start);
    if (!memory) {
        std::cerr << "Failed to allocate memory for PowerOfTwo Allocator.\n";
        return;
    }

    try {
        start = std::chrono::high_resolution_clock::now();
        PowerOfTwoAllocator allocator(memory, memorySize, minBlockSize, maxBlockSize);
        printDuration("PowerOfTwoAllocator creation", start);

        start = std::chrono::high_resolution_clock::now();
        allocator.debugPrint();
        void* block1 = allocator.alloc(20);
        // void* block2 = allocator.alloc(64);
        // void* block3 = allocator.alloc(70);
        // void* block2 = allocator.alloc(60);
        printDuration("Allocating blocks in PowerOfTwo Allocator", start);
        allocator.debugPrint();

        start = std::chrono::high_resolution_clock::now();
        allocator.free(block1);
        // allocator.free(block2);
        // allocator.free(block3);
        printDuration("Freeing blocks in PowerOfTwo Allocator", start);

        allocator.debugPrint();

        std::free(memory);
    } catch (const std::exception& e) {
        std::cerr << "PowerOfTwo Allocator Error: " << e.what() << "\n";
        std::free(memory);
    }
}

int main() {
    std::cout << "Testing MCK Allocator...\n";
    testMCKAllocator();

    std::cout << "\nTesting PowerOfTwo Allocator...\n";
    testPowerOfTwoAllocator();

    return 0;
}
