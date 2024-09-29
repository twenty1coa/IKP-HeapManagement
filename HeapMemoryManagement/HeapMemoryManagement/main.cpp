#include "Heap.h"
#include <iostream>

int main() {
    // Create a heap with an initial size of 1000 bytes, 5 threads, 3 segments, and 10 blocks per segment
    Heap myHeap(1000, 5, 3, 10);

    while (true) {
        std::cout << "1. Allocate memory\n";
        std::cout << "2. Deallocate memory\n";
        std::cout << "3. Use GarbageCollector\n";
        std::cout << "4. Measure allocation permeability\n";
        std::cout << "5. Measure deallocation permeability\n";
        std::cout << "6. Check memory\n";
        std::cout << "7. Change allocation strategy\n";
        std::cout << "8. Run Generational GC\n";
        std::cout << "9. Run Concurrent Mark-and-Sweep GC\n";
        std::cout << "10. Exit\n";
        std::cout << "Enter your choice: ";

        int choice;
        std::cin >> choice;

        switch (choice) {
        case 1: {
            std::cout << "Enter size to allocate: ";
            size_t size;
            std::cin >> size;
            std::string strategy;
            std::cout << "Enter allocation strategy (First-Fit / Best-Fit / Worst-Fit): ";
            std::cin >> strategy;
            void* allocatedMemory = myHeap.Allocate(size, strategy);
            std::cout << "Allocated memory at address: " << allocatedMemory << std::endl;
            break;
        }
        case 2: {
            std::cout << "Enter Block ID to deallocate: ";
            int blockId;
            std::cin >> blockId;

            myHeap.Deallocate(blockId);
            break;
        }
        case 3:
            std::cout << "Using Garbage Collector...\n";
            myHeap.CollectGarbage();
            myHeap.CheckMemory();
            break;
        case 4: {
            size_t allocationThreads;
            std::cout << "Enter number of threads to measure allocation permeability: ";
            std::cin >> allocationThreads;
            myHeap.MeasureAllocationPermeabilitySelective(allocationThreads);
            break;
        }
        case 5: {
            size_t deallocationThreads;
            std::cout << "Enter number of threads to measure deallocation permeability: ";
            std::cin >> deallocationThreads;
            myHeap.MeasureDeallocationPermeabilitySelective(deallocationThreads);
            break;
        }
        case 6:
            std::cout << "Checking memory...\n";
            myHeap.CheckMemory();
            break;
        case 7: {
            std::cout << "Select allocation strategy (First-Fit / Best-Fit / Worst-Fit): ";
            std::string allocationStrategy;
            std::cin >> allocationStrategy;
            if (allocationStrategy != "First-Fit" && allocationStrategy != "Best-Fit" && allocationStrategy != "Worst-Fit") {
                std::cout << "Invalid strategy! Using First-Fit by default.\n";
                allocationStrategy = "First-Fit";
            }
            std::cout << "Allocation strategy set to: " << allocationStrategy << std::endl;
            break;
        }
        case 8:
            std::cout << "Running Generational GC...\n";
            myHeap.RunGenerationalGC();
            break;
        case 9:
            std::cout << "Running Concurrent Mark-and-Sweep GC...\n";
            myHeap.RunConcurrentMarkAndSweep();
            break;
        case 10:
            return 0;
        default:
            std::cout << "Invalid choice. Please try again.\n";
            break;
        }

        std::cout << "\n";
    }
    return 0;
}
