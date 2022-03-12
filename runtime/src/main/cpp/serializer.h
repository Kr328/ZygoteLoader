#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int32_t serializer_write_full(int conn, const void *buffer, uint32_t length);
int32_t serializer_read_full(int conn, void *buffer, uint32_t length);
int32_t serializer_read_file_descriptor(int conn, int *fd);
int32_t serializer_write_file_descriptor(int conn, int fd);
int32_t serializer_read_string(int conn, char **str);
int32_t serializer_write_string(int conn, const char *str);
int32_t serializer_read_int(int conn, int *value);
int32_t serializer_write_int(int conn, const int value);

#ifdef __cplusplus
};
#endif