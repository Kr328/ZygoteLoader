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