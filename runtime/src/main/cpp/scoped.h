#pragma once

#include <stdint.h>

class ScopedFileDescriptor {
public:
    ScopedFileDescriptor(int fd);
    ~ScopedFileDescriptor();

public:
    operator int() const;

public:
    int const fd;
};

class ScopedMemoryMapping {
public:
    ScopedMemoryMapping(int fd, size_t length, int protect);
    ~ScopedMemoryMapping();

public:
    operator void *() const;

public:
    void * const base;
    size_t const length;
};

class ScopedBlocking {
public:
    ScopedBlocking(int fd);
    ~ScopedBlocking();

public:
    bool setBlocking(bool blocking) const;

private:
    const int fd;
    const bool original;
};