#pragma once

#include <jni.h>
#include <string>

#define PACKAGE_NAME_SYSTEM_SERVER ".android"

#define ZYGOTE_DEBUG_ENABLE_JDWP 1u

class ProcessUtils {
public:
    static std::string resolveProcessName(JNIEnv *env, jstring niceName);
};