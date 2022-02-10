#include "dex.h"

#include "logger.h"
#include "jni_utils.h"
#include "linker_utils.h"

#include <dlfcn.h>

typedef void (*set_java_debuggable_func)(void *runtime, bool debuggable);

void Dex::loadAndInvokeLoader(
        JNIEnv *env,
        const std::string &packageName,
        const void *dexFile, size_t dexFileLength,
        const void *propertiesFile, size_t propertiesLength,
        bool setTrusted, bool isDebuggable
) {
    jobject oSystemClassLoader = JNIUtils::getSystemClassLoader(env);
    fatal_assert(oSystemClassLoader != nullptr);

    jobject oClassLoader = JNIUtils::newInstance(
            env,
            "dalvik/system/InMemoryDexClassLoader",
            "(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V",
            env->NewDirectByteBuffer(const_cast<void*>(dexFile), static_cast<jlong>(dexFileLength)),
            oSystemClassLoader
    );
    fatal_assert(oClassLoader != nullptr);

    auto cLoader = (jclass) JNIUtils::invokeMethodObject(
            env,
            oClassLoader, "java/lang/ClassLoader",
            "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;",
            env->NewStringUTF("com.github.kr328.zloader.internal.Loader")
    );
    fatal_assert(cLoader != nullptr);

    if (setTrusted) {
        LinkerUtils linker{env, cLoader, "privilegeCallBridge"};

        auto runtimeInstance = (void **) linker.dlsym(
                RTLD_DEFAULT, "_ZN3art7Runtime9instance_E"
        );
        fatal_assert(runtimeInstance != nullptr);
        fatal_assert(*runtimeInstance != nullptr);

        auto setJavaDebuggable = (set_java_debuggable_func) linker.dlsym(
                RTLD_DEFAULT, "_ZN3art7Runtime17SetJavaDebuggableEb"
        );
        fatal_assert(setJavaDebuggable != nullptr);

        jobject oPathList = JNIUtils::getField(env, oClassLoader,
                                               "dalvik/system/BaseDexClassLoader", "pathList",
                                               "Ldalvik/system/DexPathList;");
        fatal_assert(oPathList != nullptr);

        auto oDexElements = (jobjectArray) JNIUtils::getField(env, oPathList,
                                                              "dalvik/system/DexPathList",
                                                              "dexElements",
                                                              "[Ldalvik/system/DexPathList$Element;");
        fatal_assert(oDexElements != nullptr);

        jobject oDexElement = env->GetObjectArrayElement(oDexElements, 0);
        fatal_assert(oDexElement != nullptr);

        jobject oDexFile = JNIUtils::getField(env, oDexElement, "dalvik/system/DexPathList$Element",
                                              "dexFile", "Ldalvik/system/DexFile;");
        fatal_assert(oDexFile != nullptr);

        JNIUtils::invokeMethodVoid(
                env,
                oDexFile, "dalvik/system/DexFile",
                "setTrusted", "()V"
        );

        if (!isDebuggable) {
            setJavaDebuggable(*runtimeInstance, false);
        }
    }

    auto mLoad = env->GetStaticMethodID(
            cLoader, "load", "(Ljava/lang/String;Ljava/nio/ByteBuffer;)V");
    fatal_assert(mLoad != nullptr);

    env->CallStaticVoidMethod(
            cLoader, mLoad,
            env->NewStringUTF(packageName.c_str()),
            env->NewDirectByteBuffer(const_cast<void*>(propertiesFile), static_cast<jlong>(propertiesLength))
    );
}
