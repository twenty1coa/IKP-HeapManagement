#pragma once

#include <cstddef>
#include <mutex>
#include <vector>
#include <functional>
#include <future>
#include <thread>
#include <atomic>


class Heap {
private:
    struct Block {
        size_t size = 0;
        bool marked = false;

        int blockId;
        void* allocatedMemory = nullptr;
        void* memoryPointer = nullptr;
        // Example: Assume blocks have pointers to other objects
        std::vector<Block*> pointers;
        int generation = 0;
    };

    struct Segment {
        std::vector<Block> blocks;
    };

    std::vector<Segment> segments;
    std::vector<Block> memory;
    std::mutex heapMutex;


    std::vector<void*> rootSet;
    //int blockCounter = 0;
    static const size_t maxBlocksPerSegment = 10;
    // Helper functions
    void Mark(Block& block);
    void Sweep();
    size_t totalThreads;
    void WorkerFunction(size_t threadIndex, size_t totalTasks, size_t totalThreads, std::function<void(size_t)> taskFunction);
    void RemoveFromRootSet(const Block& block);
    size_t getSegmentIndexForBlock(const Block& block) const;

    // Custom Allocation Strategies
    Block* findFirstFit(size_t size);
    Block* findBestFit(size_t size);
    Block* findWorstFit(size_t size);

    // Generational GC
    std::vector<Block*> youngGeneration;
    std::vector<Block*> oldGeneration;
    void CollectYoungGeneration();
    void CollectOldGeneration();
    void PromoteToOldGeneration(Block& block);

    // Concurrent GC
    std::atomic<bool> gcRunning = false;
    void ConcurrentMarkAndSweep();

public:
    Heap(size_t initialHeapSize, size_t totalThreads, size_t segmentsCount, size_t blocksPerSegment);
    ~Heap();

    void* Allocate(size_t size, const std::string& strategy = "First-Fit");
    void Deallocate(int blockId);
    void CollectGarbage();
    static int blockCounter;
    void CheckMemory();
    void freeMemory(Block& block);
    // Measure and output allocation permeability on specific threads
    void MeasureAllocationPermeabilitySelective(size_t totalThreads);
    // Measure and output deallocation permeability on specific threads
    void MeasureDeallocationPermeabilitySelective(size_t totalThreads);

    // New methods for advanced garbage collection
    void RunGenerationalGC();
    void RunConcurrentMarkAndSweep();
};

