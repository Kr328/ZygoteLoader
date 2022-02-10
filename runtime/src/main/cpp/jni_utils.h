#pragma once

#include <initializer_list>
#include <string>
#include <cstdarg>
#include <jni.h>

class JNIUtils {
public:
    static jobject getField(
            JNIEnv *env,
            jobject object,
            const char *className,
            const char *fieldName,
            const char *signature
    );
    static jobject newInstance(
            JNIEnv *env,
            const char *className,
            const char *constructorSignature,
            ...
    );
    static jobject invokeMethodObject(
            JNIEnv *env,
            jobject object,
            const char *className,
            const char *methodName,
            const char *signature,
            ...
    );
    static void invokeMethodVoid(
            JNIEnv *env,
            jobject object,
            const char *className,
            const char *methodName,
            const char *signature,
            ...
    );
    static jobject getSystemClassLoader(JNIEnv *env);
};
