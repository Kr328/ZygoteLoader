#include "dex.h"

#include "log.h"

#define LOADER_CLASS_NAME "com.github.kr328.zloader.internal.Loader"

#define RETURN_IF_NULL(expr) if ((expr) == nullptr) { dumpException(env); return; }

static void dumpException(JNIEnv *env) {
    if (env->ExceptionCheck()) {
        Log::e("Load dex failed");
        env->ExceptionDescribe();
        env->ExceptionClear();
        return;
    }
}

void Dex::loadAndInvokeLoader(
        JNIEnv *env,
        const std::string &dexPath,
        const std::string &packageName,
        const std::string &properties
) {
    auto cClassLoader = env->FindClass("java/lang/ClassLoader");
    RETURN_IF_NULL(cClassLoader);
    auto mSystemClassLoader = env->GetStaticMethodID(
            cClassLoader, "getSystemClassLoader", "()Ljava/lang/ClassLoader;");
    RETURN_IF_NULL(mSystemClassLoader);
    auto mLoadClass = env->GetMethodID(
            cClassLoader, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
    RETURN_IF_NULL(mLoadClass);
    auto systemClassLoader = env->CallStaticObjectMethod(cClassLoader, mSystemClassLoader);
    RETURN_IF_NULL(systemClassLoader);

    auto cDexClassLoader = env->FindClass("dalvik/system/DexClassLoader");
    RETURN_IF_NULL(cDexClassLoader);
    auto mDexClassLoader = env->GetMethodID(
            cDexClassLoader, "<init>", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;Ljava/lang/ClassLoader;)V");
    RETURN_IF_NULL(mDexClassLoader);
    auto classLoader = env->NewObject(
            cDexClassLoader,
            mDexClassLoader,
            env->NewStringUTF(dexPath.c_str()),
            static_cast<jobject>(nullptr),
            static_cast<jobject>(nullptr),
            systemClassLoader
    );
    RETURN_IF_NULL(classLoader);
    classLoader = env->NewGlobalRef(classLoader);

    auto cLoader = reinterpret_cast<jclass>(env->CallObjectMethod(
            classLoader,
            mLoadClass,
            env->NewStringUTF(LOADER_CLASS_NAME)
    ));
    RETURN_IF_NULL(cLoader);

    auto mLoad = env->GetStaticMethodID(
            cLoader, "load", "(Ljava/lang/String;Ljava/lang/String;)V");
    RETURN_IF_NULL(mLoad);

    env->CallStaticVoidMethod(
            cLoader, mLoad,
            env->NewStringUTF(packageName.c_str()),
            env->NewStringUTF(properties.c_str())
    );
    dumpException(env);
}