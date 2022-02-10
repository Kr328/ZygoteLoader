#pragma once

#include <jni.h>

class LinkerUtils {
public:
    LinkerUtils(JNIEnv *env, jclass clazz, const char *methodName);

public:
    void *dlsym(void *handle, const char *name);

private:
    JNIEnv *env;
    jclass clazz;
    jmethodID method;
};
