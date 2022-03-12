#include "process.h"

#include <stddef.h>
#include <string.h>

void process_get_package_name(JNIEnv *env, jstring process_name, char **package_name) {
    const char *str = (*env)->GetStringUTFChars(env, process_name, NULL);

    *package_name = strdup(str);

    char *split = strchr(*package_name, ':');
    if (split != NULL) {
        *split = '\0';
    }

    (*env)->ReleaseStringUTFChars(env, process_name, str);
}
