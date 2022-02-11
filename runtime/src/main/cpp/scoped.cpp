#include "scoped.h"

#include <unistd.h>
#include <fcntl.h>
#include <fcntl.h>
#include <sys/mman.h>

static void *mmapOrNull(int fd, size_t length, int protect) {
    void *result = mmap(nullptr, length, protect, MAP_SHARED, fd, 0);
    return result == MAP_FAILED ? nullptr : result;
}

ScopedFileDescriptor::ScopedFileDescriptor(int _fd) : fd(_fd) {

}

ScopedFileDescriptor::~ScopedFileDescriptor() {
    if (fd > 0) {
        close(fd);
    }
}

ScopedFileDescriptor::operator int() const {
    return fd;
}

ScopedMemoryMapping::ScopedMemoryMapping(
        int fd,
        size_t length,
        int protect
) : base(mmapOrNull(fd, length, protect)), length(length) {

}

ScopedMemoryMapping::~ScopedMemoryMapping() {
    if (base != nullptr) {
        munmap(const_cast<void*>(base), length);
    }
}

ScopedMemoryMapping::operator void *() const {
    return base;
}

ScopedBlocking::ScopedBlocking(int fd) : fd(fd), original(!(fcntl(fd, F_GETFL) & O_NONBLOCK)) {

}

ScopedBlocking::~ScopedBlocking() {
    setBlocking(original);
}

bool ScopedBlocking::setBlocking(bool blocking) const {
    int flags = fcntl(fd, F_GETFL);

    if (blocking) {
        flags = flags & (~O_NONBLOCK);
    } else {
        flags = flags | O_NONBLOCK;
    }

    return fcntl(fd, F_SETFL, flags) >= 0;
}
