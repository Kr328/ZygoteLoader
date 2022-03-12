#include "serializer.h"

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include <sys/socket.h>

#define FILE_DESCRIPTOR_OOB "fd"

int32_t serializer_read_full(int conn, void *buffer, uint32_t length) {
    uint8_t *ptr = buffer;
    uint32_t remain = length;

    while (remain > 0) {
        int32_t r = read(conn, ptr, remain);
        if (r <= 0) {
            return r;
        }

        ptr += r;
        remain -= r;
    }

    return length;
}

int32_t serializer_write_full(int conn, const void *buffer, uint32_t length) {
    const uint8_t *ptr = buffer;
    uint32_t remain = length;

    while (remain > 0) {
        int32_t r = write(conn, ptr, remain);
        if (r <= 0) {
            return r;
        }

        ptr += r;
        remain -= r;
    }

    return length;
}

int32_t serializer_read_file_descriptor(int conn, int *fd) {
    struct iovec vec;
    memset(&vec, 0, sizeof(struct iovec));

    uint8_t oob[sizeof(FILE_DESCRIPTOR_OOB)] = {0};
    vec.iov_base = oob;
    vec.iov_len = sizeof(FILE_DESCRIPTOR_OOB);

    uint8_t ctl_msg[CMSG_SPACE(sizeof(fd))];
    memset(&ctl_msg, 0, CMSG_SPACE(sizeof(fd)));

    struct msghdr msg;
    memset(&msg, 0, sizeof(struct msghdr));

    msg.msg_iov = &vec;
    msg.msg_iovlen = 1;
    msg.msg_control = ctl_msg;
    msg.msg_controllen = CMSG_SPACE(sizeof(fd));

    struct cmsghdr *ctl_hdr = CMSG_FIRSTHDR(&msg);
    ctl_hdr->cmsg_level = SOL_SOCKET;
    ctl_hdr->cmsg_type = SCM_RIGHTS;
    ctl_hdr->cmsg_len = CMSG_LEN(sizeof(fd));

    int32_t r = recvmsg(conn, &msg, 0);
    if (r < 0) {
        return r;
    }

    if (memcmp(oob, FILE_DESCRIPTOR_OOB, sizeof(FILE_DESCRIPTOR_OOB)) != 0) {
        errno = EBADF;

        return -1;
    }

    *fd = *(int *)(CMSG_DATA(ctl_hdr));

    return r;
}

int32_t serializer_write_file_descriptor(int conn, int fd) {
    struct iovec vec;
    memset(&vec, 0, sizeof(struct iovec));

    uint8_t oob[] = FILE_DESCRIPTOR_OOB;
    vec.iov_base = oob;
    vec.iov_len = sizeof(FILE_DESCRIPTOR_OOB);

    uint8_t ctl_msg[CMSG_SPACE(sizeof(fd))];
    memset(&ctl_msg, 0, CMSG_SPACE(sizeof(fd)));

    struct msghdr msg;
    memset(&msg, 0, sizeof(struct msghdr));

    msg.msg_iov = &vec;
    msg.msg_iovlen = 1;
    msg.msg_control = ctl_msg;
    msg.msg_controllen = CMSG_SPACE(sizeof(fd));

    struct cmsghdr *ctl_hdr = CMSG_FIRSTHDR(&msg);
    ctl_hdr->cmsg_level = SOL_SOCKET;
    ctl_hdr->cmsg_type = SCM_RIGHTS;
    ctl_hdr->cmsg_len = CMSG_LEN(sizeof(fd));

    *(int *)(CMSG_DATA(ctl_hdr)) = fd;

    return sendmsg(conn, &msg, 0);
}

int32_t serializer_read_string(int conn, char **str) {
    uint64_t length;
    int32_t r;

    if ((r = serializer_read_full(conn, &length, sizeof(uint64_t))) < 0) {
        return r;
    }

    *str = malloc(length + 1);

    if ((r = serializer_read_full(conn, *str, length)) < 0) {
        free(*str);

        return r;
    }

    (*str)[length] = '\0';

    return r + sizeof(uint64_t);
}

int32_t serializer_write_string(int conn, const char *str) {
    uint64_t length = strlen(str);
    int32_t r;

    if ((r = serializer_write_full(conn, &length, sizeof(uint64_t))) < 0) {
        return r;
    }

    if ((r = serializer_write_full(conn, str, length)) < 0) {
        return r;
    }

    return r + sizeof(uint64_t);
}

int32_t serializer_read_int(int conn, int *value) {
    return serializer_read_full(conn, value, sizeof(int));
}

int32_t serializer_write_int(int conn, const int value) {
    return serializer_write_full(conn, &value, sizeof(int));
}
