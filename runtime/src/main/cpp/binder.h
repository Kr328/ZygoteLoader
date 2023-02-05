#pragma once

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

int binder_interceptors_init(JNIEnv *env, jclass c_loader);

#ifdef __cplusplus
};
#endif
