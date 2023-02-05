#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void *plt_dlsym(const char *name, size_t *total);

#ifdef __cplusplus
}
#endif
