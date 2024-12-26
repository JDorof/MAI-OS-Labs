#include <cstddef>
#include <stdexcept>
#include <iostream>
#include <cstring>

class MCKAllocator {
private:
    struct Block {
        Block* next; // Указатель на следующий свободный блок
    };

    Block* freeList;     // Список свободных блоков
    void* memoryStart;   // Указатель на начало выделенной памяти
    size_t totalSize;    // Общий размер памяти
    size_t blockSize;    // Размер одного блока
    size_t blockCount;   // Количество блоков

public:
    // Конструктор аллокатора
    MCKAllocator(void* memory, size_t memorySize, size_t blockSize)
        : freeList(nullptr), memoryStart(memory), totalSize(memorySize), blockSize(blockSize) {
        if (blockSize < sizeof(Block)) {
            throw std::invalid_argument("Block size is too small");
        }

        // Инициализация свободного списка
        blockCount = totalSize / blockSize;
        freeList = reinterpret_cast<Block*>(memory);

        Block* current = freeList;
        for (size_t i = 0; i < blockCount - 1; ++i) {
            current->next = reinterpret_cast<Block*>(reinterpret_cast<char*>(current) + blockSize);
            current = current->next;
        }
        current->next = nullptr; // Последний блок указывает на nullptr
    }

    // Выделение блока памяти
    void* alloc() {
        if (!freeList) {
            throw std::bad_alloc(); // Нет свободных блоков
        }

        // Извлечение блока из списка
        Block* allocatedBlock = freeList;
        freeList = freeList->next;
        return static_cast<void*>(allocatedBlock);
    }

    // Освобождение блока памяти
    void free(void* block) {
        if (!block) {
            return; // Нельзя освобождать nullptr
        }

        // Возвращение блока в список
        Block* freedBlock = static_cast<Block*>(block);
        freedBlock->next = freeList;
        freeList = freedBlock;
    }

    // Вывод информации о состоянии аллокатора
    void debugPrint() const {
        std::cout << "MCK Allocator Debug Info:\n";
        std::cout << "Total memory: " << totalSize << " bytes\n";
        std::cout << "Block size: " << blockSize << " bytes\n";
        std::cout << "Block count: " << blockCount << "\n";

        size_t freeBlocks = 0;
        Block* current = freeList;
        while (current) {
            ++freeBlocks;
            current = current->next;
        }
        std::cout << "Free blocks: " << freeBlocks << "\n";
    }
};
