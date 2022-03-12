#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef void (*properties_for_each_block)(void *ctx, const char *key, const char *value);

void properties_for_each(const void *properties, uint32_t length, void *ctx, properties_for_each_block block);

#ifdef __cplusplus
};
#endif