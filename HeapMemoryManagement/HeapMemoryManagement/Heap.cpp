#include "Heap.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <limits>
#include <atomic>
#include <random>

int Heap::blockCounter = 0;


Heap::Heap(size_t initialHeapSize, size_t totalThreads, size_t segmentsCount, size_t blocksPerSegment)
    : totalThreads(totalThreads) {

    // Random number generator to create different block sizes
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> sizeDistribution(20, 200);  // Block sizes between 20 and 200

    // Initialize the heap with segments
    int blockCounter = 0;
    for (size_t i = 0; i < segmentsCount; ++i) {
        Segment newSegment;
        newSegment.blocks.resize(blocksPerSegment);

        // Initialize blocks within the segment with varying sizes and Block IDs
        for (size_t j = 0; j < blocksPerSegment; ++j) {
            newSegment.blocks[j].blockId = blockCounter++;  // Assign unique Block ID at creation
            newSegment.blocks[j].size = sizeDistribution(gen);  // Set block size randomly
            newSegment.blocks[j].marked = false;  // Initially, the block is free (not marked)
            newSegment.blocks[j].memoryPointer = nullptr;  // No memory allocated yet
        }

        segments.push_back(newSegment);
    }
}



Heap::~Heap() {
    for (Segment& segment : segments) {
        for (Block& block : segment.blocks) {
            freeMemory(block);
        }
    }
    segments.clear();
    rootSet.clear();
}


void* Heap::Allocate(size_t size, const std::string& strategy) {
    std::lock_guard<std::mutex> lock(heapMutex);

    Block* selectedBlock = nullptr;

    // Choose the strategy to find a block
    if (strategy == "First-Fit") {
        selectedBlock = findFirstFit(size);
    }
    else if (strategy == "Best-Fit") {
        selectedBlock = findBestFit(size);
    }
    else if (strategy == "Worst-Fit") {
        selectedBlock = findWorstFit(size);
    }

    // If a suitable block is found, allocate memory (without changing the blockId)
    if (selectedBlock) {
        selectedBlock->marked = true;
        void* allocatedMemory = malloc(size);
        selectedBlock->memoryPointer = allocatedMemory;

        std::cout << "Allocated memory at address: " << allocatedMemory
            << " (Block ID: " << selectedBlock->blockId
            << ", Strategy: " << strategy << ")\n";

        rootSet.push_back(selectedBlock);
        return allocatedMemory;
    }

    // If no suitable block is found, fall back to the existing allocation logic
    std::cerr << "No suitable block found using strategy " << strategy << ". Falling back to existing logic.\n";

    for (Segment& segment : segments) {
        if (segment.blocks.size() < maxBlocksPerSegment) {
            for (Block& block : segment.blocks) {
                if (!block.marked && block.size >= size) {
                    block.marked = true;

                    void* allocatedMemory = malloc(size);
                    block.memoryPointer = allocatedMemory;
                    std::cout << "Allocated memory at address: " << allocatedMemory
                        << " (Block ID: " << block.blockId << ")" << std::endl;

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

    // Create a new block and assign it a new blockId
    Block* newBlock = new Block;  // Allocate dynamically
    newBlock->size = size;
    newBlock->marked = true;
    newBlock->blockId = ++blockCounter;  // Assign a new Block ID for the new block

    void* allocatedMemory = malloc(size);
    newBlock->memoryPointer = allocatedMemory;

    std::cout << "Allocated memory at address: " << allocatedMemory
        << " (Block ID: " << newBlock->blockId << ")" << std::endl;

    newSegment->blocks.push_back(*newBlock);
    rootSet.push_back(newBlock);
    return allocatedMemory;
}



void Heap::Deallocate(int blockId) {
    std::lock_guard<std::mutex> lock(heapMutex);
    for (Segment& segment : segments) {
        for (Block& block : segment.blocks) {
            if (block.blockId == blockId && block.marked) {
                RemoveFromRootSet(block);
                block.marked = false;
                freeMemory(block);
                std::cout << "Deallocated memory with Block ID: " << blockId << std::endl;
                return;
            }
        }
    }
    std::cerr << "Deallocate failed: Block ID " << blockId << " not found or already deallocated.\n";
}

Heap::Block* Heap::findFirstFit(size_t size) {
    for (Segment& segment : segments) {
        for (Block& block : segment.blocks) {
            std::cout << "Checking Block ID: " << block.blockId
                << " | Marked: " << block.marked
                << " | Size: " << block.size
                << " | Memory Pointer: " << block.memoryPointer << "\n";
            if (!block.marked && block.size >= size) {
                std::cout << "First-Fit selected Block ID: " << block.blockId << "\n";
                return &block;
            }
        }
    }
    return nullptr;
}



Heap::Block* Heap::findBestFit(size_t size) {
    Block* bestFit = nullptr;
    size_t smallestSize = std::numeric_limits<size_t>::max();

    std::cout << "Running Best-Fit strategy...\n";
    for (Segment& segment : segments) {
        for (Block& block : segment.blocks) {
            std::cout << "Checking Block ID: " << block.blockId
                << " | Marked: " << block.marked
                << " | Size: " << block.size
                << " | Memory Pointer: " << block.memoryPointer << "\n";

            if (!block.marked && block.size >= size && block.size < smallestSize) {
                smallestSize = block.size;
                bestFit = &block;
                std::cout << "Current Best-Fit candidate Block ID: " << block.blockId
                    << " with size: " << block.size << "\n";
            }
        }
    }

    if (bestFit) {
        std::cout << "Best-Fit selected Block ID: " << bestFit->blockId
            << " with size: " << bestFit->size << "\n";
    }
    else {
        std::cerr << "No suitable block found using Best-Fit strategy.\n";
    }

    return bestFit;
}

Heap::Block* Heap::findWorstFit(size_t size) {
    Block* worstFit = nullptr;
    size_t largestSize = 0;  // Initialize with 0 to find the largest block

    std::cout << "Running Worst-Fit strategy...\n";
    for (Segment& segment : segments) {
        for (Block& block : segment.blocks) {
            std::cout << "Checking Block ID: " << block.blockId
                << " | Marked: " << block.marked
                << " | Size: " << block.size
                << " | Memory Pointer: " << block.memoryPointer << "\n";

            if (!block.marked && block.size >= size) {
                if (block.size > largestSize) {
                    largestSize = block.size;
                    worstFit = &block;
                    std::cout << "Current Worst-Fit candidate Block ID: " << block.blockId
                        << " with size: " << block.size << "\n";
                }
            }
        }
    }

    if (worstFit) {
        std::cout << "Worst-Fit selected Block ID: " << worstFit->blockId
            << " with size: " << worstFit->size << "\n";
    }
    else {
        std::cerr << "No suitable block found using Worst-Fit strategy.\n";
    }

    return worstFit;
}


void Heap::freeMemory(Block& block) {
    if (block.allocatedMemory) {
        free(block.allocatedMemory);
        block.allocatedMemory = nullptr;
    }
}

void Heap::RemoveFromRootSet(const Block& block) {
    rootSet.erase(std::remove_if(rootSet.begin(), rootSet.end(),
        [&block](const void* root) {
            const Block* rootBlock = static_cast<const Block*>(root);
            return rootBlock && rootBlock->blockId == block.blockId;
        }), rootSet.end());
}

/*
void Heap::CollectGarbage() {
    std::lock_guard<std::mutex> lock(heapMutex);
    for (void* root : rootSet) {
        Mark(*reinterpret_cast<Block*>(root));
    }
    Sweep();
}

void Heap::Mark(Block& block) {
    if (block.marked) return;
    block.marked = true;
    for (Block* ptr : block.pointers) {
        if (ptr && !ptr->marked) {
            Mark(*ptr);
        }
    }
}

void Heap::Sweep() {
    for (Segment& segment : segments) {
        auto newEnd = std::remove_if(segment.blocks.begin(), segment.blocks.end(),
            [](const Block& block) { return !block.marked; });

        for (auto it = newEnd; it != segment.blocks.end(); ++it) {
            RemoveFromRootSet(*it);
            freeMemory(*it);
        }
        segment.blocks.erase(newEnd, segment.blocks.end());

        for (Block& block : segment.blocks) {
            block.marked = false;
        }
    }
}
*/
void Heap::CollectGarbage() {
    std::lock_guard<std::mutex> lock(heapMutex);

    // Mark phase: Mark all reachable objects from the root set
    std::cout << "Starting garbage collection...\n";
    for (void* root : rootSet) {
        if (root) {
            Mark(*reinterpret_cast<Block*>(root));
        }
    }

    // Sweep phase: Free memory that is not marked
    Sweep();
    std::cout << "Garbage collection complete.\n";
}

void Heap::Mark(Block& block) {
    // If the block is already marked, skip it
    if (block.marked) return;

    // Mark the current block as reachable
    block.marked = true;

    // Recursively mark objects referenced by pointers in this block (if any)
    for (Block* referencedBlock : block.pointers) {
        if (referencedBlock && !referencedBlock->marked) {
            Mark(*referencedBlock);
        }
    }
}

void Heap::Sweep() {
    for (Segment& segment : segments) {
        auto newEnd = std::remove_if(segment.blocks.begin(), segment.blocks.end(),
            [](const Block& block) {
                // Blocks that are unmarked should be removed (freed)
                return !block.marked;
            });

        // Free memory for swept (unmarked) blocks
        for (auto it = newEnd; it != segment.blocks.end(); ++it) {
            freeMemory(*it);
        }

        // Remove swept blocks from the segment
        segment.blocks.erase(newEnd, segment.blocks.end());

        // Reset the marked flag for the remaining blocks (marked blocks)
        for (Block& block : segment.blocks) {
            block.marked = false;  // Reset for the next garbage collection cycle
        }
    }
}

size_t Heap::getSegmentIndexForBlock(const Block& block) const {
    for (size_t i = 0; i < segments.size(); ++i) {
        const Segment& segment = segments[i];
        auto it = std::find_if(segment.blocks.begin(), segment.blocks.end(),
            [&block](const Block& segBlock) {
                return block.blockId == segBlock.blockId;
            });
        if (it != segment.blocks.end()) {
            return i;
        }
    }
    return SIZE_MAX;
}

void Heap::CheckMemory() {
    std::lock_guard<std::mutex> lock(heapMutex);
    std::cout << "Checking memory integrity...\n";

    for (size_t i = 0; i < segments.size(); ++i) {
        const Segment& segment = segments[i];
        std::cout << "Segment " << i << ":\n";

        for (const Block& block : segment.blocks) {
            if (block.marked) {
                std::cout << "  Block at address " << block.memoryPointer
                    << " (Block ID: " << block.blockId
                    << ", Size: " << block.size << ") is allocated.\n";
            }
            else {
                std::cout << "  Block at address " << block.memoryPointer
                    << " (Block ID: " << block.blockId
                    << ", Size: " << block.size << ") is deallocated.\n";
            }
        }
    }

    std::cout << "Root set:\n";
    for (void* root : rootSet) {
        const Block* block = reinterpret_cast<const Block*>(root);
        if (block) {
            size_t segmentIndex = getSegmentIndexForBlock(*block);
            if (segmentIndex != SIZE_MAX) {
                std::cout << "Root at address " << root
                    << " points to Block ID: " << block->blockId
                    << " (Size: " << block->size << ") in Segment " << segmentIndex << ".\n";
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



void Heap::RunGenerationalGC() {
    std::lock_guard<std::mutex> lock(heapMutex);
    std::cout << "Running generational garbage collection...\n";

    CollectYoungGeneration();

    static int oldCollectionCount = 0;
    oldCollectionCount++;
    if (oldCollectionCount >= 5) {
        CollectOldGeneration();
        oldCollectionCount = 0;
    }

    std::cout << "Generational garbage collection complete.\n";
}

void Heap::CollectYoungGeneration() {
    for (Block* block : youngGeneration) {
        if (!block->marked) {
            Mark(*block);
        }
    }
    Sweep();

    for (auto it = youngGeneration.begin(); it != youngGeneration.end();) {
        if ((*it)->marked) {
            PromoteToOldGeneration(**it);
            it = youngGeneration.erase(it);
        }
        else {
            ++it;
        }
    }
}

void Heap::CollectOldGeneration() {
    for (Block* block : oldGeneration) {
        if (!block->marked) {
            Mark(*block);
        }
    }
    Sweep();
}

void Heap::PromoteToOldGeneration(Block& block) {
    block.generation = 1;
    oldGeneration.push_back(&block);
}

void Heap::RunConcurrentMarkAndSweep() {
    if (gcRunning.exchange(true)) {
        std::cerr << "Concurrent garbage collection already running!\n";
        return;
    }

    std::cout << "Starting concurrent mark-and-sweep...\n";
    std::thread gcThread([this]() {
        std::lock_guard<std::mutex> lock(heapMutex);
        for (void* root : rootSet) {
            Mark(*reinterpret_cast<Block*>(root));
        }
        Sweep();
        gcRunning = false;
        std::cout << "Concurrent mark-and-sweep complete.\n";
        });

    gcThread.detach();
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
    size_t totalTasks = 5;

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
    size_t totalTasks = 5;

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
