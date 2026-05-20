#define _DEFAULT_SOURCE
#include <sys/mman.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

#define INTEGRETY_CHECK 123456

typedef struct __freenode_t {
    size_t size;
    struct __freenode_t* next;
} __freenode_t;

typedef struct __header_t {
    size_t size;
    int i;
} __header_t;

__freenode_t* __head = NULL;

int start();
void *mal(size_t size);
void free(void *ptr);

size_t __cursize = 4080;
int start() {
    __head = (__freenode_t*) mmap(NULL, __cursize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, 0, 0);
    if (__head == MAP_FAILED){
        return 1;
    }

    __head->size = __cursize;
    __head->next = NULL;

    return 0;
}

void printfreelist() {
    for (__freenode_t* cur = __head; cur != NULL; cur = cur->next){
        printf("[%p] size: %ld; next %p\n", cur, cur->size, cur->next);
    }
}

void *mal(size_t size) {
    size_t alloc_size = sizeof(__header_t) + size;
    if (__head == NULL && start() == 1) {
        return NULL;
    }

    __freenode_t *cur = __head;
    __freenode_t *prev = NULL;
    while (cur != NULL && cur->size < alloc_size){
        prev = cur;
        cur = cur->next;
    }

    // TODO: request more memory from OS later
    if (cur == NULL){
        return NULL;
    }

    //split

    // no point on splitting a full allocation
    if (alloc_size == cur->size){
        if (cur == __head){
            __head = cur->next;
        }else {
            prev->next = cur->next;
        }
    }else {
        __freenode_t* newnode = (__freenode_t*) (cur + alloc_size);
        newnode->next = cur->next;
        newnode->size = cur->size - sizeof(__header_t) - size;
        if (prev != NULL){
            prev->next = newnode;
        }else{
            __head = newnode;
        }
    }

    __header_t* header = (__header_t*) cur;
    header->i = INTEGRETY_CHECK;
    header->size = size;

    #ifdef DEBUG
    printfreelist();
    #endif

    void* returned_ptr = (uint8_t*)header + sizeof(__header_t);

    return returned_ptr;
}

void mfr(void *ptr) {
    size_t header_size = sizeof(__header_t);
    __header_t* header = ptr - header_size;

    assert(header->i == INTEGRETY_CHECK);

    size_t newsize = header->size + sizeof(__header_t);
    __freenode_t* newnode = (__freenode_t*) header;
    newnode->size = newsize;
    newnode->next = NULL;

    __freenode_t* cur = __head;
    __freenode_t* prev = NULL;
    while (cur != NULL && cur->size < newsize){
        prev = cur;
        cur = cur->next;
    }

    if (prev == NULL){
        newnode->next = __head;
        __head = newnode;
        return;
    }

    prev->next = newnode;
    newnode->next = cur;
}