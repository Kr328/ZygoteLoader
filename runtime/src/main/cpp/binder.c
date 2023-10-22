#include "binder.h"

#include "logger.h"
#include "ext/plt.h"

#include <malloc.h>
#include <string.h>
#include <stdarg.h>

static jclass c_binder;
static jmethodID m_exec_transact;
static jmethodID m_get;
static jfieldID f_native_ptr;
static jobject o_redirect_binders;

static const struct JNINativeInterface *original_jni_env;

static jboolean replaced_call_boolean_method_v(
        JNIEnv *env,
        jobject obj,
        jmethodID method,
        va_list args
) {
    if (method != m_exec_transact || obj == NULL ||
        !original_jni_env->IsInstanceOf(env, obj, c_binder)) {
        return original_jni_env->CallBooleanMethodV(env, obj, method, args);
    }

    original_jni_env->MonitorEnter(env, o_redirect_binders);

    jobject o_target = original_jni_env->CallObjectMethod(env, o_redirect_binders, m_get, obj);

    original_jni_env->MonitorExit(env, o_redirect_binders);

    if (o_target != NULL) {
        return original_jni_env->CallBooleanMethodV(env, o_target, method, args);
    }

    return original_jni_env->CallBooleanMethodV(env, obj, method, args);
}

static jboolean JNICALL invoke_next_call_boolean_method(
        JNIEnv *env,
        jobject obj,
        jmethodID method,
        ...
) {
    va_list args;
    va_start(args, method);
    jboolean ret = original_jni_env->CallBooleanMethodV(env, obj, method, args);
    va_end(args);
    return ret;
}

static jboolean JNICALL jni_call_exec_transact(
        JNIEnv *env,
        jclass clazz,
        jobject binder,
        jint code,
        jobject data,
        jobject reply,
        jint flags
) {
    jlong data_ptr = 0;
    if (data != NULL) {
        data_ptr = original_jni_env->GetLongField(env, data, f_native_ptr);
    }

    jlong reply_ptr = 0;
    if (reply != NULL) {
        reply_ptr = original_jni_env->GetLongField(env, data, f_native_ptr);
    }

    return invoke_next_call_boolean_method(
            env,
            binder,
            m_exec_transact,
            code,
            data_ptr,
            reply_ptr,
            flags
    );
}

int binder_interceptors_init(JNIEnv *env, jclass c_loader) {
    void (*set_jni_env)(struct JNINativeInterface *) = plt_dlsym(
            "_ZN3art9JNIEnvExt16SetTableOverrideEPK18JNINativeInterface", NULL);
    if (set_jni_env == NULL) {
        return 1;
    }

    c_binder = (*env)->FindClass(env, "android/os/Binder");
    if (c_binder == NULL) {
        return 1;
    }

    m_exec_transact = (*env)->GetMethodID(env, c_binder, "execTransact", "(IJJI)Z");
    if (m_exec_transact == NULL) {
        return 1;
    }

    jclass c_parcel = (*env)->FindClass(env, "android/os/Parcel");
    if (c_parcel == NULL) {
        return 1;
    }

    f_native_ptr = (*env)->GetFieldID(env, c_parcel, "mNativePtr", "J");
    if (f_native_ptr == NULL) {
        return 1;
    }

    jclass c_map = (*env)->FindClass(env, "java/util/Map");
    if (c_map == NULL) {
        return 1;
    }

    m_get = (*env)->GetMethodID(env, c_map, "get", "(Ljava/lang/Object;)Ljava/lang/Object;");
    if (m_get == NULL) {
        return 1;
    }

    jfieldID f_redirect_binders = (*env)->GetStaticFieldID(
            env, c_loader, "redirectBinders", "Ljava/util/WeakHashMap;");
    if (f_redirect_binders == NULL) {
        return 1;
    }

    o_redirect_binders = (*env)->GetStaticObjectField(env, c_loader, f_redirect_binders);
    if (o_redirect_binders == NULL) {
        return 1;
    }

    c_binder = (*env)->NewGlobalRef(env, c_binder);
    o_redirect_binders = (*env)->NewGlobalRef(env, o_redirect_binders);

    original_jni_env = *env;

    struct JNINativeInterface *replaced_jni_env = malloc(sizeof(struct JNINativeInterface));
    memcpy(replaced_jni_env, original_jni_env, sizeof(struct JNINativeInterface));
    replaced_jni_env->CallBooleanMethodV = &replaced_call_boolean_method_v;

    set_jni_env(replaced_jni_env);

    static JNINativeMethod methods[] = {
            {
                    .name = "nativeCallExecTransact",
                    .signature = "(Landroid/os/Binder;ILandroid/os/Parcel;Landroid/os/Parcel;I)Z",
                    .fnPtr = &jni_call_exec_transact,
            }
    };

    if ((*env)->RegisterNatives(env, c_loader, methods, sizeof(methods) / sizeof(*methods))) {
        return 1;
    }

    return 0;
}