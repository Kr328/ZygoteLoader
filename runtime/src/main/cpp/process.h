#pragma once

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PACKAGE_NAME_SYSTEM_SERVER ".android"

#define ZYGOTE_DEBUG_ENABLE_JDWP 1u

void process_get_package_name(JNIEnv *env, jstring process_name, char **package_name);

#ifdef __cplusplus
};
#endif