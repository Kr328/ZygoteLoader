#include "mem.h"

#include <malloc.h>

void operator delete(void *block) {
    free(block);
}

void *operator new(size_t size) {
    return malloc(size);
}
