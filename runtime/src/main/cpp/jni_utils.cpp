#include "jni_utils.h"

#include <cstdlib>
#include <cstdarg>

jobject JNIUtils::getField(
        JNIEnv *env,
        jobject object,
        const char *className,
        const char *fieldName,
        const char *signature
) {
    jclass clazz = env->FindClass(className);
    if (clazz == nullptr) {
        return nullptr;
    }

    jfieldID field = env->GetFieldID(clazz, fieldName, signature);
    if (field == nullptr) {
        return nullptr;
    }

    return env->GetObjectField(object, field);
}

jobject JNIUtils::getSystemClassLoader(JNIEnv *env) {
    jclass clazz = env->FindClass("java/lang/ClassLoader");
    if (clazz == nullptr) {
        return nullptr;
    }

    jmethodID method = env->GetStaticMethodID(clazz, "getSystemClassLoader",
                                              "()Ljava/lang/ClassLoader;");
    if (method == nullptr) {
        return nullptr;
    }

    return env->CallStaticObjectMethod(clazz, method);
}

jobject
JNIUtils::newInstance(
        JNIEnv *env,
        const char *className,
        const char *constructorSignature,
        ...
) {
    jclass clazz = env->FindClass(className);
    if (clazz == nullptr) {
        return nullptr;
    }

    jmethodID constructor = env->GetMethodID(clazz, "<init>", constructorSignature);
    if (constructor == nullptr) {
        return nullptr;
    }

    va_list args;
    va_start(args, constructorSignature);
    jobject result = env->NewObjectV(clazz, constructor, args);
    va_end(args);

    return result;
}

jobject JNIUtils::invokeMethodObject(
        JNIEnv *env,
        jobject object,
        const char *className,
        const char *methodName,
        const char *signature,
        ...
) {
    jclass clazz = env->FindClass(className);
    if (clazz == nullptr) {
        return nullptr;
    }

    jmethodID method = env->GetMethodID(clazz, methodName, signature);
    if (method == nullptr) {
        return nullptr;
    }

    va_list args;
    va_start(args, signature);
    jobject result = env->CallObjectMethodV(object, method, args);
    va_end(args);

    return result;
}

void JNIUtils::invokeMethodVoid(
        JNIEnv *env,
        jobject object,
        const char *className,
        const char *methodName,
        const char *signature,
        ...
) {
    jclass clazz = env->FindClass(className);
    if (clazz == nullptr) {
        return;
    }

    jmethodID method = env->GetMethodID(clazz, methodName, signature);
    if (method == nullptr) {
        return;
    }

    va_list args;
    va_start(args, signature);
    env->CallVoidMethodV(object, method, args);
    va_end(args);
}
