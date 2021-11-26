#pragma once

#include <cstdint>
#include <string>

namespace IO {
    int fileSize(int fd);
    int readFull(int fd, void *buffer, int size);
    int writeFull(int fd, const void *buffer, int size);
    uint8_t *readFile(const std::string &path, int &size);
}