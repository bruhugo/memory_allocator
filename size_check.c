
#include <stdio.h>
#include <stdint.h>
typedef enum { TYPE_NODE, TYPE_HEADER } __BlockType;
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
int main() {
    printf("freenode: %zu\n", sizeof(__freenode_t));
    printf("header: %zu\n", sizeof(__header_t));
    printf("btag: %zu\n", sizeof(__bound_tag_t));
    return 0;
}
