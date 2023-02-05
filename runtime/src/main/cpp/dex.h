#pragma once

#include <jni.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void dex_load_and_invoke(
        JNIEnv *env,
        const char *package_name,
        const void *dex_block, uint32_t dex_length,
        const void *properties_block, uint32_t properties_length,
        int use_binder_interceptors
);

#ifdef __cplusplus
};
#endif
