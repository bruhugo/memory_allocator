#include "./allocator.c"

int main() {
    int* ptr1 = mal(8);
    int* ptr2 = mal(16);
    int* ptr3 = mal(32);
    int* ptr4 = mal(64);

    mfr(ptr1);
    mfr(ptr2);
    mfr(ptr3);
    mfr(ptr4);

    printfreelist();
}
