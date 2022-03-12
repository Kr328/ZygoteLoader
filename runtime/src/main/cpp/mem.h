#pragma once

#include <stddef.h>

void operator delete(void *block);
void *operator new(size_t size);
