#include "dex.h"

#include "logger.h"

#define LOADER_CLASS_NAME "com.github.kr328.zloader.internal.Loader"

#define RETURN_IF_NULL(expr) if ((expr) == nullptr) { dumpException(env); return; }

static void dumpException(JNIEnv *env) {
    if (env->ExceptionCheck()) {
        jthrowable throwable = env->ExceptionOccurred();

        env->ExceptionClear();

        jmethodID mToString = env->GetMethodID(env->FindClass("java/lang/Object"),
                                               "toString",
                                               "()Ljava/lang/String;");
        auto description = reinterpret_cast<jstring>(env->CallObjectMethod(throwable, mToString));

        if (env->ExceptionCheck()) {
            env->ExceptionClear();
        }

        const char *descriptionString = env->GetStringUTFChars(description, nullptr);
        Logger::e("Load dex failed: " + std::string(descriptionString));
        env->ReleaseStringUTFChars(description, descriptionString);

        return;
    }
}

void Dex::loadAndInvokeLoader(Chunk *file, JNIEnv *env,
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

    auto cInMemoryClassLoader = env->FindClass("dalvik/system/InMemoryDexClassLoader");
    RETURN_IF_NULL(cInMemoryClassLoader);
    auto mInMemoryClassLoader = env->GetMethodID(
            cInMemoryClassLoader, "<init>", "(Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V");
    RETURN_IF_NULL(mInMemoryClassLoader);
    auto classLoader = env->NewObject(
            cInMemoryClassLoader,
            mInMemoryClassLoader,
            env->NewDirectByteBuffer(file->getData(), file->getLength()),
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
