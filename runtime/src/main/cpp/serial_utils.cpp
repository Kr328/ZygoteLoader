#include "serial_utils.h"

#include <cstring>
#include <unistd.h>
#include <sys/socket.h>

#define FILE_DESCRIPTOR_OOB "fd"

ssize_t SerialUtils::readFull(int fd, void *buffer, size_t size) {
    auto ptr = static_cast<uint8_t *>(buffer);
    auto remain = size;

    while (remain > 0) {
        ssize_t r = read(fd, ptr, remain);
        if (r < 0) {
            return r;
        }

        ptr += r;
        remain -= r;
    }

    return static_cast<int>(size);
}

ssize_t SerialUtils::writeFull(int fd, const void *buffer, size_t size) {
    auto ptr = static_cast<const uint8_t *>(buffer);
    auto remain = size;

    while (remain > 0) {
        ssize_t r = write(fd, ptr, remain);
        if (r < 0) {
            return r;
        }

        ptr += r;
        remain -= r;
    }

    return static_cast<int>(size);
}

ssize_t SerialUtils::writeFileDescriptor(int conn, int fd) {
    struct iovec vec{};
    memset(&vec, 0, sizeof(struct iovec));

    uint8_t oob[] = FILE_DESCRIPTOR_OOB;
    vec.iov_base = oob;
    vec.iov_len = sizeof(FILE_DESCRIPTOR_OOB);

    uint8_t ctlMsg[CMSG_SPACE(sizeof(fd))];
    memset(&ctlMsg, 0, CMSG_SPACE(sizeof(fd)));

    struct msghdr msg{};
    memset(&msg, 0, sizeof(struct msghdr));

    msg.msg_iov = &vec;
    msg.msg_iovlen = 1;
    msg.msg_control = ctlMsg;
    msg.msg_controllen = CMSG_SPACE(sizeof(fd));

    struct cmsghdr *ctlHdr = CMSG_FIRSTHDR(&msg);
    ctlHdr->cmsg_level = SOL_SOCKET;
    ctlHdr->cmsg_type = SCM_RIGHTS;
    ctlHdr->cmsg_len = CMSG_LEN(sizeof(fd));

    *reinterpret_cast<int *>(CMSG_DATA(ctlHdr)) = fd;

    return sendmsg(conn, &msg, 0);
}

ssize_t SerialUtils::readFileDescriptor(int conn, int &fd) {
    struct iovec vec{};
    memset(&vec, 0, sizeof(struct iovec));

    uint8_t oob[sizeof(FILE_DESCRIPTOR_OOB)] = {0};
    vec.iov_base = oob;
    vec.iov_len = sizeof(FILE_DESCRIPTOR_OOB);

    uint8_t ctlMsg[CMSG_SPACE(sizeof(fd))];
    memset(&ctlMsg, 0, CMSG_SPACE(sizeof(fd)));

    struct msghdr msg{};
    memset(&msg, 0, sizeof(struct msghdr));

    msg.msg_iov = &vec;
    msg.msg_iovlen = 1;
    msg.msg_control = ctlMsg;
    msg.msg_controllen = CMSG_SPACE(sizeof(fd));

    struct cmsghdr *ctlHdr = CMSG_FIRSTHDR(&msg);
    ctlHdr->cmsg_level = SOL_SOCKET;
    ctlHdr->cmsg_type = SCM_RIGHTS;
    ctlHdr->cmsg_len = CMSG_LEN(sizeof(fd));

    ssize_t r = recvmsg(conn, &msg, 0);
    if (r < 0) {
        return r;
    }

    if (memcmp(oob, FILE_DESCRIPTOR_OOB, sizeof(FILE_DESCRIPTOR_OOB)) != 0) {
        errno = EBADF;

        return -1;
    }

    fd = *reinterpret_cast<int *>(CMSG_DATA(ctlHdr));

    return r;
}

ssize_t SerialUtils::writeString(int conn, const std::string &str) {
    uint64_t size = str.length();
    ssize_t rr = 0;

    if (ssize_t r = writeFull(conn, &size, sizeof(uint64_t)); r < 0) {
        return r;
    } else {
        rr += r;
    }

    if (ssize_t r = writeFull(conn, str.c_str(), size); r < 0) {
        return r;
    } else {
        rr += r;
    }

    return rr;
}

ssize_t SerialUtils::readString(int conn, std::string &str) {
    uint64_t size = 0;
    ssize_t rr = 0;

    if (ssize_t r = readFull(conn, &size, sizeof(uint64_t)); r < 0) {
        return r;
    } else {
        rr += r;
    }

    std::unique_ptr<char> data{new char[size]};

    if (ssize_t r = readFull(conn, data.get(), sizeof(size)); r < 0) {
        return r;
    } else {
        rr += r;
    }

    str = std::string(data.get(), size);

    return rr;
}

ssize_t SerialUtils::writeInt(int conn, int value) {
    return writeFull(conn, &value, sizeof(int));
}

ssize_t SerialUtils::readInt(int conn, int &value) {
    return readFull(conn, &value, sizeof(int));
}
