#pragma once

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PACKAGE_NAME_SYSTEM_SERVER ".android"

void process_get_package_name(JNIEnv *env, jstring process_name, char **package_name);

#ifdef __cplusplus
};
#endif