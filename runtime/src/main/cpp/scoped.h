#pragma once

class ScopedFileDescriptor {
public:
    ScopedFileDescriptor(int fd);
    ~ScopedFileDescriptor();

public:
    operator int() const;

public:
    int fd;
};
