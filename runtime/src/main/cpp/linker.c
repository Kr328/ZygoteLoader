#include "linker.h"

#include <dlfcn.h>

extern void *privilege_call_bridge(JNIEnv *env, jclass clazz, void *func, void *arg1, void *arg2);

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

int linker_init(struct linker_t *linker, JNIEnv *env, jclass clazz, const char *method_name) {
#if __LP64__
    const char *signature = "(JJJ)J";
#else
    const char *signature = "(III)I";
#endif

    JNINativeMethod methods[] = {
            {
                    .name = method_name,
                    .signature = signature,
                    .fnPtr = (void *) &privilege_call_bridge,
            }
    };

    if ((*env)->RegisterNatives(env, clazz, methods, 1) != JNI_OK) {
        return -1;
    }

    linker->env = env;
    linker->clazz = clazz;
    linker->method = (*env)->GetStaticMethodID(env, clazz, method_name, signature);

    if (linker->method == NULL) {
        return -1;
    }

    return 0;
}

void *linker_find_symbol(struct linker_t *linker, void *handle, const char *symbol) {
#if defined(__LP64__)
    return (void *) (*linker->env)->CallStaticLongMethod(
            linker->env,
            linker->clazz,
            linker->method,
            (jlong) &dlsym,
            (jlong) handle,
            (jlong) symbol
    );
#else
    return (void *) (*linker->env)->CallStaticIntMethod(
            linker->env,
            linker->clazz,
            linker->method,
            (jint) &dlsym,
            (jint) handle,
            (jint) symbol
    );
#endif
}