#include "dex.h"

#include "logger.h"

#include <dlfcn.h>

#define LOADER_CLASS_NAME "com.github.kr328.zloader.internal.Loader"

#define RETURN_IF_NULL(expr) do {if ((expr) == nullptr) {dumpNull(env, #expr);return false;}} while (0)

extern "C" {
typedef void (*set_java_debuggable_func)(void *runtime, bool value);

void *privilege_call_bridge(JNIEnv *env, jclass clazz, void *func, void *arg1, void *arg2);
#if defined(__aarch64__)
__asm__("privilege_call_bridge:\n"
        "mov x0, x3\n"
        "mov x1, x4\n"
        "br  x2\n");
#elif defined(__arm__)
__asm__("privilege_call_bridge:\n"
        ".arm\n"
        "mov r0, r3\n"
        "ldr r1, [sp]\n"
        "bx  r2\n");
#elif defined(__x86_64__)
__asm__("privilege_call_bridge:\n"
        "movq %rdx, %rax\n"
        "movq %rcx, %rdi\n"
        "movq %r8, %rsi\n"
        "jmpq *%rax\n");
#elif defined(__i686__)
__asm__("privilege_call_bridge:\n"
        "movl 20(%esp), %eax\n"
        "movl %eax, 8(%esp)\n"
        "movl 16(%esp), %eax\n"
        "movl %eax, 4(%esp)\n"
        "movl 12(%esp), %eax\n"
        "jmpl *%eax");
#else
#error "Unsupported arch"
#endif
}

static void dumpNull(JNIEnv *env, const char *msg) {
    Logger::e(msg + std::string(" == nullptr"));

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
        Logger::e(descriptionString);
        env->ReleaseStringUTFChars(description, descriptionString);

        return;
    }
}

static jmethodID registerPrivilegeCallBridge(JNIEnv *env, jclass clazz) {
#if defined(__LP64__)
    JNINativeMethod methods[] = {{
        .name = "privilegeCallBridge64",
        .signature = "(JJJ)J",
        .fnPtr = reinterpret_cast<void*>(&privilege_call_bridge),
    }};

    env->RegisterNatives(clazz, methods, 1);

    return env->GetStaticMethodID(clazz, "privilegeCallBridge64", "(JJJ)J");
#else
    JNINativeMethod methods[] = {{
        .name = "privilegeCallBridge32",
        .signature = "(III)I",
        .fnPtr = reinterpret_cast<void*>(&privilege_call_bridge),
        }};

    env->RegisterNatives(clazz, methods, 1);

    return env->GetStaticMethodID(clazz, "privilegeCallBridge32", "(III)I");
#endif
}

static void *resolvePrivilegeSymbol(JNIEnv *env, jclass cLoader, jmethodID mPrivilege, const char *symbol) {
#if defined(__LP64__)
    return (void *) env->CallStaticLongMethod(cLoader, mPrivilege, (jlong) &dlsym, (jlong) RTLD_DEFAULT, (jlong) symbol);
#else
    return (void *) env->CallStaticIntMethod(cLoader, mPrivilege, (jint) &dlsym, (jint) RTLD_DEFAULT, (jint) symbol);
#endif
}

static bool setClassLoaderTrusted(JNIEnv *env, void *runtime, set_java_debuggable_func setJavaDebuggable, jobject oClassLoader, bool isDebuggable) {
    jclass cBaseDexClassLoader = env->FindClass("dalvik/system/BaseDexClassLoader");
    RETURN_IF_NULL(cBaseDexClassLoader);

    jfieldID fPathList = env->GetFieldID(cBaseDexClassLoader, "pathList", "Ldalvik/system/DexPathList;");
    RETURN_IF_NULL(fPathList);

    jobject oPathList = env->GetObjectField(oClassLoader, fPathList);
    RETURN_IF_NULL(oPathList);

    jclass cDexPathList = env->FindClass("dalvik/system/DexPathList");
    RETURN_IF_NULL(cDexPathList);

    jfieldID fDexElements = env->GetFieldID(cDexPathList, "dexElements", "[Ldalvik/system/DexPathList$Element;");
    RETURN_IF_NULL(fDexElements);

    auto oDexElements = reinterpret_cast<jobjectArray>(env->GetObjectField(oPathList, fDexElements));
    RETURN_IF_NULL(oDexElements);

    jclass cDexPathList$Element = env->FindClass("dalvik/system/DexPathList$Element");
    RETURN_IF_NULL(cDexPathList$Element);

    jfieldID fDexFile = env->GetFieldID(cDexPathList$Element, "dexFile", "Ldalvik/system/DexFile;");
    RETURN_IF_NULL(fDexFile);

    jclass cDexFile = env->FindClass("dalvik/system/DexFile");
    RETURN_IF_NULL(cDexFile);

    jmethodID mSetTrusted = env->GetMethodID(cDexFile, "setTrusted", "()V");
    RETURN_IF_NULL(mSetTrusted);

    setJavaDebuggable(runtime, true);

    int length = env->GetArrayLength(oDexElements);
    for (int i = 0; i < length; i++) {
        jobject oElement = env->GetObjectArrayElement(oDexElements, i);
        if (oElement == nullptr)
            continue;

        jobject oDexFile = env->GetObjectField(oElement, fDexFile);
        RETURN_IF_NULL(oDexFile);

        env->CallVoidMethod(oDexFile, mSetTrusted);
        if (env->ExceptionCheck()) {
            dumpNull(env, "setTrusted");
        }
    }

    if (!isDebuggable) {
        setJavaDebuggable(runtime, false);
    }

    return true;
}

bool Dex::loadAndInvokeLoader(Chunk *file, JNIEnv *env,
                              const std::string &packageName,
                              const std::string &properties,
                              bool setTrusted, bool isDebuggable
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

    if (setTrusted) {
        jmethodID mPrivilege = registerPrivilegeCallBridge(env, cLoader);
        RETURN_IF_NULL(mPrivilege);

        auto runtimeInstance = reinterpret_cast<void**>(resolvePrivilegeSymbol(env, cLoader, mPrivilege, "_ZN3art7Runtime9instance_E"));
        RETURN_IF_NULL(runtimeInstance);
        RETURN_IF_NULL(*runtimeInstance);

        auto setJavaDebuggable = reinterpret_cast<set_java_debuggable_func>(resolvePrivilegeSymbol(env, cLoader, mPrivilege, "_ZN3art7Runtime17SetJavaDebuggableEb"));
        RETURN_IF_NULL(setJavaDebuggable);

        if (!setClassLoaderTrusted(env, *runtimeInstance, setJavaDebuggable, classLoader, isDebuggable)) {
            return false;
        }
    }

    auto mLoad = env->GetStaticMethodID(
            cLoader, "load", "(Ljava/lang/String;Ljava/lang/String;)V");
    RETURN_IF_NULL(mLoad);

    env->CallStaticVoidMethod(
            cLoader, mLoad,
            env->NewStringUTF(packageName.c_str()),
            env->NewStringUTF(properties.c_str())
    );

    return true;
}
