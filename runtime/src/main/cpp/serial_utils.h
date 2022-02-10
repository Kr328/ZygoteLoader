#pragma once

#include <string>
#include <cstdint>

class SerialUtils {
public:
    static ssize_t writeFull(int conn, const void *buffer, size_t length);
    static ssize_t readFull(int conn, void *buffer, size_t length);

public:
    static ssize_t writeFileDescriptor(int conn, int fd);
    static ssize_t readFileDescriptor(int conn, int &fd);

public:
    static ssize_t writeString(int conn, std::string const &str);
    static ssize_t readString(int conn, std::string &str);

public:
    static ssize_t writeInt(int conn, int value);
    static ssize_t readInt(int conn, int &value);
};
