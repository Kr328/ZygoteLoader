#include "linker_utils.h"

#include "logger.h"

#include <dlfcn.h>

extern "C"
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

LinkerUtils::LinkerUtils(JNIEnv *env, jclass clazz, const char *methodName) : env(env), clazz(clazz) {
#if __LP64__
    JNINativeMethod methods[] = {
            {
                    .name = methodName,
                    .signature = "(JJJ)J",
                    .fnPtr = (void *) &privilege_call_bridge,
            }
    };

    env->RegisterNatives(clazz, methods, 1);

    method = env->GetStaticMethodID(clazz, methodName, "(JJJ)J");
#else
    JNINativeMethod methods[] = {
                {
                        .name = methodName,
                        .signature = "(III)I",
                        .fnPtr = (void *) &privilege_call_bridge,
                }
    };

    env->RegisterNatives(clazz, methods, 1);

    method = env->GetStaticMethodID(clazz, methodName, "(III)I");
#endif

    fatal_assert(method != nullptr);
}

void *LinkerUtils::dlsym(void *handle, const char *symbol) {
#if defined(__LP64__)
    return (void *) env->CallStaticLongMethod(clazz, method, (jlong) &::dlsym,
                                              (jlong) handle, (jlong) symbol);
#else
    return (void *) env->CallStaticIntMethod(clazz, method, (jint) &::dlsym,
                                                 (jint) handle, (jint) symbol);
#endif
}
