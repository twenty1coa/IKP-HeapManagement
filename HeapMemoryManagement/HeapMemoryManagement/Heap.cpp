#include "Heap.h"
#include <iostream>
#include <chrono>
#include <thread>


Heap::Heap(size_t initialHeapSize, size_t totalThreads, size_t segmentsCount, size_t blocksPerSegment)
    : totalThreads(totalThreads) {
    // Divide the initial heap size among the threads
    size_t baseSize = initialHeapSize / totalThreads;
    size_t remainder = initialHeapSize % totalThreads;

    // Initialize the heap with segments
    for (size_t i = 0; i <= segmentsCount; ++i) {
        Segment newSegment;
        // Use blocksPerSegment instead of segmentSize
        newSegment.blocks.resize(blocksPerSegment);
        for (size_t j = 0; j < blocksPerSegment; ++j) {
            newSegment.blocks[j].blockId = j;  // Set block IDs based on position in the segment
        }

        segments.push_back(newSegment);
    }
}

Heap::~Heap() {
    // Clean up resources if needed
    for (Segment& segment : segments) {
        for (Block& block : segment.blocks) {
            // Free memory associated with each block
            freeMemory(block);
        }
    }

    // Clear the segments and root set
    segments.clear();
    rootSet.clear();
}

void* Heap::Allocate(size_t size) {
    std::lock_guard<std::mutex> lock(heapMutex);

    // Find a free block that is large enough for the allocation
    for (Segment& segment : segments) {
        if (segment.blocks.size() < maxBlocksPerSegment) {
            for (Block& block : segment.blocks) {
                if (!block.marked && block.size >= size) {
                    block.marked = true;
                    block.blockId = ++blockCounter;

                    void* allocatedMemory = malloc(size);
                    block.memoryPointer = allocatedMemory;
                    std::cout << "Allocated memory at address: " << allocatedMemory << " (Block ID: " << block.blockId << ")" << std::endl;

                    rootSet.push_back(&block);
                    return allocatedMemory;
                }
            }
        }
    }

    // If no suitable block is found in existing segments, allocate in a new segment
    Segment* newSegment = nullptr;
    for (Segment& segment : segments) {
        if (segment.blocks.size() < maxBlocksPerSegment) {
            newSegment = &segment;
            break;
        }
    }

    if (newSegment == nullptr) {
        // No existing segment has available space, create a new segment
        segments.emplace_back();
        newSegment = &segments.back();
    }

    Block* newBlock = new Block;  // Allocate dynamically
    newBlock->size = size;
    newBlock->marked = true;
    newBlock->blockId = ++blockCounter;

    void* allocatedMemory = malloc(size);
    newBlock->memoryPointer = allocatedMemory;

    std::cout << "Allocated memory at address: " << allocatedMemory << " (Block ID: " << newBlock->blockId << ")" << std::endl;

    newSegment->blocks.push_back(*newBlock);

    rootSet.push_back(newBlock);
    return allocatedMemory;
}
void Heap::Deallocate(int blockId) {
    std::lock_guard<std::mutex> lock(heapMutex);

    // Find the block with the specified blockId in any segment
    for (Segment& segment : segments) {
        auto it = std::find_if(segment.blocks.begin(), segment.blocks.end(), [blockId](const Block& block) {
            return block.blockId == blockId && block.marked;
            });

        if (it != segment.blocks.end()) {
            // Remove pointers from the deallocated block from the rootSet
            RemoveFromRootSet(*it);

            // Mark the block as not allocated
            it->marked = false;

            std::cout << "Deallocated memory with Block ID: " << blockId << std::endl;
            return;
        }
    }

    std::cerr << "Error: Block with Block ID " << blockId << " not found or already deallocated.\n";
}


void Heap::CheckMemory() {
    std::lock_guard<std::mutex> lock(heapMutex);

    std::cout << "Checking memory integrity...\n";

    for (size_t i = 0; i < segments.size(); ++i) {
        const Segment& segment = segments[i];
        std::cout << "Segment " << i << ":\n";

        for (const Block& block : segment.blocks) {
            if (block.marked) {
                std::cout << "  Block at address " << block.memoryPointer << " (Block ID: " << block.blockId << ") is allocated.\n";
            }
            else {
                std::cout << "  Block at address " << block.memoryPointer << " (Block ID: " << block.blockId << ") is deallocated.\n";
            }
        }
    }

    std::cout << "Root set:\n";
    for (void* root : rootSet) {
        const Block* block = reinterpret_cast<const Block*>(root);
        if (block) {
            size_t segmentIndex = getSegmentIndexForBlock(*block);
            if (segmentIndex != SIZE_MAX) {
                std::cout << "Root at address " << root << " points to Block ID: " << block->blockId
                    << " in Segment " << segmentIndex << ".\n";
            }
            else {
                std::cerr << "Error: Block not found in any segment.\n";
            }
        }
        else {
            std::cerr << "Error: Null pointer in root set.\n";
        }
    }

    std::cout << "Memory check complete.\n";
}

size_t Heap::getSegmentIndexForBlock(const Block& block) const {
    // Function to find the segment index for a given block
    for (size_t i = 0; i < segments.size(); ++i) {
        const Segment& segment = segments[i];
        auto it = std::find_if(segment.blocks.begin(), segment.blocks.end(),
            [&block](const Block& segBlock) {
                return block.blockId == segBlock.blockId;
            });

        if (it != segment.blocks.end()) {
            return i;  // Found the segment
        }
    }

    return SIZE_MAX;  // Block not found in any segment
}

void Heap::CollectGarbage() {
    std::lock_guard<std::mutex> lock(heapMutex);

    // Mark phase: Mark all reachable objects from the root set
    for (void* root : rootSet) {
        if (root) {
            Mark(*reinterpret_cast<Block*>(root));
        }
    }
    // Sweep phase: Free memory that is not marked
    Sweep();
}

void Heap::Mark(Block& block) {
    // Mark the current block
    block.marked = true;

    // Recursively mark objects referenced by pointers in this block
    for (Block* referencedBlock : block.pointers) {
        if (referencedBlock && !referencedBlock->marked) {
            Mark(*referencedBlock);
        }
    }
}

void Heap::Sweep() {
    for (Segment& segment : segments) {
        auto newEnd = std::remove_if(segment.blocks.begin(), segment.blocks.end(),
            [](const Block& block) { return !block.marked; });

        // Remove references to swept blocks from rootSet
        for (auto it = newEnd; it != segment.blocks.end(); ++it) {
            RemoveFromRootSet(*it);
        }

        // Free memory for swept blocks
        for (auto it = newEnd; it != segment.blocks.end(); ++it) {
            freeMemory(*it);
        }

        segment.blocks.erase(newEnd, segment.blocks.end());

        // Reset the marked flag for the remaining blocks in the segment
        for (Block& block : segment.blocks) {
            block.marked = false;
        }
    }
}
void Heap::RemoveFromRootSet(const Block& block) {
    rootSet.erase(std::remove_if(rootSet.begin(), rootSet.end(),
        [&block](const void* root) {
            const Block* rootBlock = static_cast<const Block*>(root);
            return rootBlock && rootBlock->blockId == block.blockId;
        }),
        rootSet.end());
}

void Heap::freeMemory(Block& block) {
    free(block.memoryPointer);
}

void Heap::WorkerFunction(size_t threadIndex, size_t totalTasks, size_t totalThreads, std::function<void(size_t)> taskFunction) {
    for (size_t i = 0; i < totalTasks; ++i) {
        if (i % totalThreads == threadIndex) {
            taskFunction(i);
        }
    }
}

void Heap::MeasureAllocationPermeabilitySelective(size_t totalThreads) {
    auto startTime = std::chrono::high_resolution_clock::now();

    std::vector<std::future<void>> futures;
    size_t totalTasks = 5; // Adjust the total number of tasks as needed

    for (size_t i = 0; i < totalThreads; ++i) {
        futures.emplace_back(std::async(std::launch::async, [this, i, totalTasks, totalThreads]() {
            WorkerFunction(i, totalTasks, totalThreads, [this](size_t taskIndex) {
                size_t size = (taskIndex % 100) + 1;
                void* allocatedMemory = Allocate(size);
                });
            }));
    }

    for (auto& future : futures) {
        future.wait();
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    std::cout << "Selective Allocation Permeability (" << totalThreads << " threads): "
        << totalTasks * totalThreads << " allocations in " << duration << " ms\n";
}

void Heap::MeasureDeallocationPermeabilitySelective(size_t totalThreads) {
    auto startTime = std::chrono::high_resolution_clock::now();

    std::vector<std::future<void>> futures;
    size_t totalTasks = 5; // Adjust the total number of tasks as needed

    for (size_t i = 0; i < totalThreads; ++i) {
        futures.emplace_back(std::async(std::launch::async, [this, i, totalTasks, totalThreads]() {
            WorkerFunction(i, totalTasks, totalThreads, [this](size_t taskIndex) {
                int blockId = taskIndex + 1;
                Deallocate(blockId);
                });
            }));
    }

    for (auto& future : futures) {
        future.wait();
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    std::cout << "Selective Deallocation Permeability (" << totalThreads << " threads): "
        << totalTasks * totalThreads << " deallocations in " << duration << " ms\n";
}

int Heap::blockCounter = 0;