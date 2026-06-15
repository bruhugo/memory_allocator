## Memory Allocator in C

This library allocates and manages memory in a C application. 
It requests memory from the OS by using the mmap() system call and keeps track of 
available memory by using a linked list (free list data structure).

### Usage

```
#define DEBUG
#include "./allocator.c"

int main() {
    int* ptr4 = mal(64);
    printfreelist();

    mfr(ptr4);
    printfreelist();
}
```

You call mal(64) to allocate 64 bytes of memory from the requested memory section from your operating system. 
In order to free that memory, you call mfr(ptr) to free that space and allow other allocations to use it. Please note that 
freeing the memory DOES NOT zero it, so allocated memory using mal() might contain previous memory contents.

You can define the debug directive to allow the library to print debug messages to stdout. 

MAL stands for memory allocation

MFR stands for memory free

