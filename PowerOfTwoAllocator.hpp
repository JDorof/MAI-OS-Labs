#include <cstddef>
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <cstring>

class PowerOfTwoAllocator {
// private:
public:
    struct Block {
        Block* next;
    };

    std::vector<Block*> freeLists;
    void* memoryStart;
    size_t totalSize;
    size_t minBlockSize;
    size_t maxBlockSize;

    size_t nearestPowerOfTwo(size_t blockSize) {
        size_t power = 1;
        while (power < blockSize) {
            power *= 2;
        }
        return power;
    }

    size_t getFreeListIndex(size_t blockSize) {
        size_t index = 0;
        while ((minBlockSize << index) < blockSize) {
            ++index;
        }
        return index;
    }

public:
    PowerOfTwoAllocator(void* memory, size_t memorySize, size_t minSize, size_t maxSize)
        : memoryStart(memory), totalSize(memorySize), minBlockSize(minSize), maxBlockSize(maxSize) {
        if (minBlockSize < sizeof(Block)) {
            throw std::invalid_argument("Minimum block size is too small");
        }

        size_t levels = static_cast<size_t>(std::log2(maxBlockSize / minBlockSize)) + 1;
        freeLists.resize(levels, nullptr);

        char* current = static_cast<char*>(memoryStart);
        while (current < static_cast<char*>(memoryStart) + totalSize) {
            size_t blockIndex = getFreeListIndex(maxBlockSize);

            for (size_t i = freeLists.size() - 1; i != static_cast<size_t>(-1); --i) {
                size_t blockSize = minBlockSize << i;
                if (static_cast<size_t>(current - static_cast<char*>(memoryStart)) + blockSize <= totalSize) {
                    Block* newBlock = reinterpret_cast<Block*>(current);
                    newBlock->next = freeLists[i];
                    freeLists[i] = newBlock;
                    current += blockSize;
                }
            }
        }

    }


    void* alloc(size_t blockSize) {
        blockSize = nearestPowerOfTwo(blockSize);
        std::cout << blockSize << std::endl;
        if (blockSize < minBlockSize || blockSize > maxBlockSize) {
            throw std::bad_alloc();
        }
        size_t index = getFreeListIndex(blockSize);
        for (size_t i = index; i < freeLists.size(); ++i) {
            if (freeLists[i]) {
                Block* block = freeLists[i];
                freeLists[i] = block->next;

                if (i > index) {
                    while (i > index) {
                        --i;
                        size_t newSize = minBlockSize << i;
                        Block* buddy = reinterpret_cast<Block*>(reinterpret_cast<char*>(block) + newSize);
                        buddy->next = freeLists[i];
                        freeLists[i] = buddy;
                    }
                }
                return static_cast<void*>(block);
            }
        }
        throw std::bad_alloc();
    }

void free(void* ptr) {
    if (!ptr) return;

    char* block = static_cast<char*>(ptr);

    for (size_t i = freeLists.size() - 1; i != static_cast<size_t>(-1); --i) {
        size_t blockSize = minBlockSize << i;

        if (reinterpret_cast<std::uintptr_t>(block) % blockSize == 0) {
            std::uintptr_t blockAddr = reinterpret_cast<std::uintptr_t>(block);
            std::uintptr_t buddyAddr = blockAddr ^ blockSize;
            Block* buddy = reinterpret_cast<Block*>(buddyAddr);

            Block* prev = nullptr;
            Block* current = freeLists[i];

            while (current) {
                if (current == buddy) {
                    if (prev) {
                        prev->next = current->next;
                    } else {
                        freeLists[i] = current->next;
                    }
                    block = reinterpret_cast<char*>(std::min(blockAddr, buddyAddr));
                    ++i;
                    break;
                }
                prev = current;
                current = current->next;
            }

            Block* newBlock = reinterpret_cast<Block*>(block);
            newBlock->next = freeLists[i];
            freeLists[i] = newBlock;
            return;
        }
    }

    throw std::invalid_argument("Pointer does not belong to any valid block.");
}


    void debugPrint() const {
        std::cout << "PowerOfTwo Allocator Debug Info:\n";
        std::cout << "Total memory: " << totalSize << " bytes\n";
        std::cout << "Min block size: " << minBlockSize << " bytes\n";
        std::cout << "Max block size: " << maxBlockSize << " bytes\n";

        for (size_t i = 0; i < freeLists.size(); ++i) {
            size_t blockSize = minBlockSize << i;
            size_t count = 0;
            Block* current = freeLists[i];
            while (current) {
                ++count;
                current = current->next;
            }
            std::cout << "Block size " << blockSize << ": " << count << " free blocks\n";
        }
    }
};
