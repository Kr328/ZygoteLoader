#include "dex.h"

#include "binder.h"
#include "logger.h"

#include <stddef.h>
#include <dlfcn.h>
#include <string.h>

#define find_class(var_name, name) jclass var_name = (*env)->FindClass(env, name); fatal_assert((var_name) != NULL)
#define find_static_method(var_name, clazz, name, signature) jmethodID var_name = (*env)->GetStaticMethodID(env, clazz, name, signature); fatal_assert((var_name) != NULL)
#define find_method(var_name, clazz, name, signature) jmethodID var_name = (*env)->GetMethodID(env, clazz, name, signature); fatal_assert((var_name) != NULL)
#define new_string(text) (*env)->NewStringUTF(env, text)

void dex_load_and_invoke(
        JNIEnv *env,
        const char *package_name,
        const void *dex_block, uint32_t dex_length,
        const void *properties_block, uint32_t properties_length,
        int use_binder_interceptors
) {
    find_class(c_class_loader, "java/lang/ClassLoader");
    find_static_method(
            m_get_system_class_loader,
            c_class_loader,
            "getSystemClassLoader",
            "()Ljava/lang/ClassLoader;"
    );

    jobject o_system_class_loader = (*env)->CallStaticObjectMethod(
            env,
            c_class_loader,
            m_get_system_class_loader
    );
    fatal_assert(o_system_class_loader != NULL);

    find_class(c_dex_class_loader, "dalvik/system/InMemoryDexClassLoader");
    find_method(
            m_dex_class_loader,
            c_dex_class_loader,
            "<init>",
            "(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V"
    );
    jobject o_dex_class_loader = (*env)->NewObject(
            env,
            c_dex_class_loader,
            m_dex_class_loader,
            (*env)->NewDirectByteBuffer(env, (void *) dex_block, dex_length),
            o_system_class_loader
    );
    fatal_assert(o_dex_class_loader != NULL);

    find_method(
            m_load_class,
            c_class_loader,
            "loadClass",
            "(Ljava/lang/String;)Ljava/lang/Class;"
    );

    jclass c_loader = (*env)->CallObjectMethod(
            env,
            o_dex_class_loader,
            m_load_class,
            new_string("com.github.kr328.zloader.internal.Loader")
    );
    fatal_assert(c_loader != NULL);

    if (use_binder_interceptors) {
        LOGD("Enable binder interceptors");

        int interceptor_initialized = binder_interceptors_init(env, c_loader);
        fatal_assert(interceptor_initialized == 0);
    }

    find_static_method(m_load, c_loader, "load", "(Ljava/lang/String;Ljava/nio/ByteBuffer;Z)V");

    (*env)->CallStaticVoidMethod(
            env,
            c_loader,
            m_load,
            new_string(package_name),
            (*env)->NewDirectByteBuffer(env, (void *) properties_block, properties_length),
            (jboolean) use_binder_interceptors
    );
}
