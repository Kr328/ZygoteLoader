#include "scoped.h"

#include <unistd.h>
#include <fcntl.h>

ScopedFileDescriptor::ScopedFileDescriptor(int _fd) {
    fd = _fd;
}

ScopedFileDescriptor::~ScopedFileDescriptor() {
    if (fd > 0) {
        close(fd);
    }
}

ScopedFileDescriptor::operator int() const {
    return fd;
}