#define _DEFAULT_SOURCE
#include <sys/mman.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

typedef enum {
    TYPE_NODE, TYPE_HEADER
} __BlockType;

typedef struct __freenode_t {
    __BlockType type;
    size_t size;
    struct __freenode_t* next;
    struct __freenode_t* prev;
} __freenode_t;

typedef struct __header_t {
    __BlockType type;
    size_t size;
} __header_t;

typedef size_t __bound_tag_t;

typedef union {
    __BlockType type;
    __freenode_t n;
    __header_t h;
} __block;

__freenode_t* __head = NULL;
size_t __cursize = 4080;
void * __root_ptr = NULL;

int start();
void *mal(size_t size);
void mfr(void *ptr);

int start() {
    __root_ptr = mmap(NULL, __cursize, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANON, 0, 0);
    __head = (__freenode_t*) __root_ptr;
    if (__head == MAP_FAILED){
        return 1;
    }

    __head->size = __cursize;
    __head->next = NULL;
    __head->prev = NULL;
    __head->type = TYPE_NODE;

    __bound_tag_t* tag = (uint8_t*)__head + __cursize - sizeof(__bound_tag_t);
    *tag = __cursize - sizeof(__bound_tag_t);

    return 0;
}

void printfreelist() {
    for (__freenode_t* cur = __head; cur != NULL; cur = cur->next){
        __bound_tag_t* tag = (uint8_t*)cur + cur->size - sizeof(__bound_tag_t);
        printf("[%p] size: %ld; btag: %ld; next %p\n", cur, cur->size, *tag, cur->next);
    }
    printf("\n");
}

void coalesce(__freenode_t* node) {
    __block* nextblock = (uint8_t*)node + node->size;
    __bound_tag_t* prevbtag = (uint8_t*)node - sizeof(__bound_tag_t);
    if (prevbtag >= (__bound_tag_t*)__root_ptr){
        size_t size = *prevbtag;
        __block* block = (uint8_t*)prevbtag - size;
        if (block >= (__block*)__root_ptr && block->type == TYPE_NODE){
            __freenode_t* newnode = (__freenode_t*)block;
            newnode->size += node->size;
            
            // remove node from list since it is now part of newnode
            if (node->prev != NULL) {
                node->prev->next = node->next;
            } else {
                __head = node->next;
            }
            if (node->next != NULL) {
                node->next->prev = node->prev;
            }

            node = newnode;

            __bound_tag_t* tag = (uint8_t*)node + node->size - sizeof(__bound_tag_t);
            *tag = node->size - sizeof(__bound_tag_t);
        }
    }

    if (nextblock >= (__block*)((uint8_t*)__root_ptr + __cursize) || nextblock->type == TYPE_HEADER) {
        return;
    } 

    __freenode_t* nextnode = (__freenode_t*)nextblock;
    __freenode_t* prev = nextnode->prev;
    if (prev == NULL) {
        __head = nextnode->next;
    }else {
        prev->next = nextnode->next;
    }

    if (nextnode->next != NULL){
        nextnode->next->prev = prev;
    }

    node->size += nextnode->size;

    __bound_tag_t* tag = (uint8_t*)node + node->size - sizeof(__bound_tag_t);
    *tag = node->size - sizeof(__bound_tag_t);
}

void *mal(size_t size) {
    size_t alloc_size = sizeof(__header_t) + sizeof(__bound_tag_t) + size;
    if (alloc_size < sizeof(__freenode_t) + sizeof(__bound_tag_t)) {
        alloc_size = sizeof(__freenode_t) + sizeof(__bound_tag_t);
    }

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
    if (cur->size - alloc_size <= sizeof(__freenode_t) + sizeof(__bound_tag_t)){
        if (cur->next != NULL){
            cur->next->prev = cur->prev;
        }

        if (cur == __head){
            __head = __head->next;
        }else {
            prev->next = cur->next;
        }
        alloc_size = cur->size;
    }else {
        __freenode_t* newnode = (__freenode_t*) ((int8_t*)cur + alloc_size);
        newnode->next = cur->next;
        newnode->prev = cur->prev;
        newnode->size = cur->size - alloc_size;
        newnode->type = TYPE_NODE;

        if (prev != NULL){
            prev->next = newnode;
        }else{
            __head = newnode;
        }

        if (cur->next != NULL){
            cur->next->prev = newnode;
        }

        __bound_tag_t* tag = (uint8_t*)newnode + newnode->size - sizeof(__bound_tag_t);
        *tag = newnode->size - sizeof(__bound_tag_t);
    }

    __header_t* header = (__header_t*) cur;
    header->size = alloc_size;
    header->type = TYPE_HEADER;

    __bound_tag_t* tag = (uint8_t*)header + header->size - sizeof(__bound_tag_t);
    *tag = alloc_size - sizeof(__bound_tag_t);

    #ifdef DEBUG
    printfreelist();
    #endif

    void* returned_ptr = (uint8_t*)header + sizeof(__header_t);

    return returned_ptr;
}

void mfr(void *ptr) {
    if (ptr == NULL) return;
    size_t header_size = sizeof(__header_t);
    __header_t* header = (__header_t*)((uint8_t*)ptr - header_size);

    __freenode_t* newnode = (__freenode_t*) header;
    newnode->size = header->size;

    // find the first node that is greater than the newsize or null
    __freenode_t* cur = __head;
    __freenode_t* prev = NULL;
    while (cur != NULL && cur->size < newnode->size){
        prev = cur;
        cur = cur->next;
    }

    // safely assigned since cur is always the next 
    // and prev is always the prev
    newnode->next = cur;
    newnode->prev = prev;
    newnode->type = TYPE_NODE;

    // if prev is null it means cur is head
    if (prev == NULL){
        __head = newnode;
    }else {
        prev->next = newnode;
    }

    // if cur is null it means no node is greater than
    // the freed space, so it must be placed at the end
    if (cur == NULL){
        newnode->prev = prev;
    }else {
        cur->prev = newnode;
    }

    coalesce(newnode);

    #ifdef DEBUG
    printfreelist();
    #endif 
}