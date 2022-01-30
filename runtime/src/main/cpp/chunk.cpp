#include "chunk.h"

#include <malloc.h>

Chunk::Chunk(uint32_t length) {
    this->data = new uint8_t[length];
    this->length = length;
}

Chunk::~Chunk() {
    delete[] this->data;
}

int Chunk::getLength() const {
    return length;
}

const void *Chunk::getData() const {
    return data;
}

void *Chunk::getData() {
    return data;
}
