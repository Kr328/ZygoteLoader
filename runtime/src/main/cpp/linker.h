#pragma once

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

struct linker_t {
    JNIEnv *env;
    jclass clazz;
    jmethodID method;
};

int linker_init(struct linker_t *linker, JNIEnv *env, jclass clazz, const char *method_name);
void *linker_find_symbol(struct linker_t *linker, void *handle, const char *symbol);

#ifdef __cplusplus
};
#endif