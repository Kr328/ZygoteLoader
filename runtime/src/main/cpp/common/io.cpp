#include "io.h"

#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

int IO::fileSize(int fd) {
    struct stat s{};

    if (fstat(fd, &s) < 0) {
        return -1;
    }

    return static_cast<int>(s.st_size);
}

int IO::readFull(int fd, void *buffer, int size) {
    auto *ptr = static_cast<uint8_t *>(buffer);
    auto remain = size;

    while (remain > 0) {
        int r = read(fd, ptr, remain);
        if (r < 0) {
            return r;
        }

        ptr += r;
        remain -= r;
    }

    return size;
}

int IO::writeFull(int fd, const void *buffer, int size) {
    auto *ptr = static_cast<const uint8_t *>(buffer);
    auto remain = size;

    while (remain > 0) {
        int r = write(fd, ptr, remain);
        if (r < 0) {
            return r;
        }

        ptr += r;
        remain += r;
    }

    return size;
}

uint8_t *IO::readFile(const std::string &path, int &size) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        return nullptr;
    }

    size = fileSize(fd);
    if (size < 0) {
        close(fd);

        return nullptr;
    }

    auto result = new uint8_t[size];
    if (IO::readFull(fd, result, size) < 0) {
        close(fd);
        delete[] result;

        return nullptr;
    }

    close(fd);

    return result;
}