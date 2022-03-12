#include "dex.h"

#include "logger.h"
#include "linker.h"

#include <stddef.h>
#include <dlfcn.h>

#define find_class(var_name, name) jclass var_name = (*env)->FindClass(env, name); fatal_assert(var_name != NULL)
#define find_static_method(var_name, clazz, name, signature) jmethodID var_name = (*env)->GetStaticMethodID(env, clazz, name, signature); fatal_assert(var_name != NULL)
#define find_method(var_name, clazz, name, signature) jmethodID var_name = (*env)->GetMethodID(env, clazz, name, signature); fatal_assert(var_name != NULL)
#define find_field(var_name, clazz, name, signature) jfieldID var_name = (*env)->GetFieldID(env, clazz, name, signature)
#define new_string(text) (*env)->NewStringUTF(env, text)

typedef void (*set_java_debuggable_func)(void *runtime, int debuggable);

void dex_load_and_invoke(
        JNIEnv *env,
        const char *package_name,
        const void *dex_block, uint32_t dex_length,
        const void *properties_block, uint32_t properties_length,
        int set_trusted, int debuggable
) {
    find_class(c_class_loader, "java/lang/ClassLoader");
    find_static_method(m_get_system_class_loader, c_class_loader, "getSystemClassLoader",
                       "()Ljava/lang/ClassLoader;");
    jobject o_system_class_loader = (*env)->CallStaticObjectMethod(
            env,
            c_class_loader,
            m_get_system_class_loader
    );
    fatal_assert(o_system_class_loader != NULL);

    find_class(c_dex_class_loader, "dalvik/system/InMemoryDexClassLoader");
    find_method(m_dex_class_loader, c_dex_class_loader, "<init>",
                "(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V");
    jobject o_dex_class_loader = (*env)->NewObject(
            env,
            c_dex_class_loader,
            m_dex_class_loader,
            (*env)->NewDirectByteBuffer(env, (void *) dex_block, dex_length),
            o_system_class_loader
    );
    fatal_assert(o_dex_class_loader != NULL);

    find_method(m_load_class, c_class_loader, "loadClass",
                "(Ljava/lang/String;)Ljava/lang/Class;");
    jclass c_loader = (*env)->CallObjectMethod(
            env,
            o_dex_class_loader,
            m_load_class,
            new_string("com.github.kr328.zloader.internal.Loader")
    );
    fatal_assert(c_loader != NULL);

    if (set_trusted) {
        struct linker_t linker;
        fatal_assert(linker_init(&linker, env, c_loader, "privilegeCallBridge") == 0);

        void **runtime = linker_find_symbol(&linker, RTLD_DEFAULT,
                                            "_ZN3art7Runtime9instance_E");
        fatal_assert(runtime != NULL);
        fatal_assert(*runtime != NULL);

        set_java_debuggable_func set_java_debuggable = linker_find_symbol(&linker, RTLD_DEFAULT,
                                                                          "_ZN3art7Runtime17SetJavaDebuggableEb");
        fatal_assert(set_java_debuggable != NULL);

        find_class(c_base_dex_class_loader, "dalvik/system/BaseDexClassLoader");
        find_field(f_path_list, c_base_dex_class_loader, "pathList", "Ldalvik/system/DexPathList;");
        jobject o_path_list = (*env)->GetObjectField(env, o_dex_class_loader, f_path_list);
        fatal_assert(o_path_list != NULL);

        find_class(c_dex_path_list, "dalvik/system/DexPathList");
        find_field(f_dex_elements, c_dex_path_list, "dexElements",
                   "[Ldalvik/system/DexPathList$Element;");
        jobjectArray o_dex_elements = (*env)->GetObjectField(env, o_path_list, f_dex_elements);
        fatal_assert(o_dex_elements != NULL);

        jobject o_dex_element = (*env)->GetObjectArrayElement(env, o_dex_elements, 0);
        fatal_assert(o_dex_elements != NULL);

        find_class(c_dex_element, "dalvik/system/DexPathList$Element");
        find_field(f_dex_file, c_dex_element, "dexFile", "Ldalvik/system/DexFile;");
        jobject o_dex_file = (*env)->GetObjectField(env, o_dex_element, f_dex_file);
        fatal_assert(o_dex_file != NULL);

        find_class(c_dex_file, "dalvik/system/DexFile");
        find_method(m_set_trusted, c_dex_file, "setTrusted", "()V");

        set_java_debuggable(*runtime, 1);

        (*env)->CallVoidMethod(env, o_dex_file, m_set_trusted);

        if (!debuggable) {
            set_java_debuggable(*runtime, 0);
        }
    }

    find_static_method(m_load, c_loader, "load", "(Ljava/lang/String;Ljava/nio/ByteBuffer;)V");

    (*env)->CallStaticVoidMethod(
            env,
            c_loader,
            m_load,
            new_string(package_name),
            (*env)->NewDirectByteBuffer(env, (void *) properties_block, properties_length)
    );
}