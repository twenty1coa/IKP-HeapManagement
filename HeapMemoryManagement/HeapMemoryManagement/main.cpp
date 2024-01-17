#include "Heap.h"
#include <iostream>

int main() {
    // Create a heap with an initial size of 1000 bytes
    Heap myHeap(1000, 5, 3, 0);

    
    while (true) {
        std::cout << "1. Allocate memory\n";
        std::cout << "2. Deallocate memory\n";
        std::cout << "3. Use GarbageCollector\n";
        std::cout << "4. Izracunaj propusnost prilikom alokacije\n";
        std::cout << "5. Izracunaj propusnost prilikom dealokacije\n";
        std::cout << "6. Check memory\n";
        std::cout << "7. Exit\n";
        std::cout << "Enter your choice: ";

        int choice;
        std::cin >> choice;

        switch (choice) {
        case 1: {
            std::cout << "Enter size to allocate: ";
            size_t size;
            std::cin >> size;

            void* allocatedMemory = myHeap.Allocate(size);
            std::cout << "Allocated memory at address: " << allocatedMemory << std::endl;
            //myHeap.CheckMemory();
            break;
        }
        case 2: {
            std::cout << "Enter Block ID to deallocate: ";
            int blockId;
            std::cin >> blockId;

            myHeap.Deallocate(blockId);
            //myHeap.CheckMemory();
            break;
            
        }
        case 3:
            std::cout << "Using Garbage Collector...\n";
            myHeap.CollectGarbage();
            //myHeap.CollectGarbage();
            myHeap.CheckMemory();
            break;
        case 4: {
            size_t allocationThreads;
            std::cout << "Unesite broj niti da izmerite propusnost alokacije: ";
            std::cin >> allocationThreads;
            myHeap.MeasureAllocationPermeabilitySelective(allocationThreads);
            break;
        }
        case 5: {
            size_t deallocationThreads;
            std::cout << "Unesite broj niti da izmerite propusnost dealokacije: ";
            std::cin >> deallocationThreads;
            myHeap.MeasureDeallocationPermeabilitySelective(deallocationThreads);
            break;
        }
        case 6:
            std::cout << "Check memory...\n";
            myHeap.CheckMemory();
            break;
        case 7:
            return 0;
        default:
            std::cout << "Invalid choice. Please try again.\n";
            break;
        }

        std::cout << "\n";
    }
    return 0;
    
}