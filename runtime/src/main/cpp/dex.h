#pragma once

#include <string>
#include <jni.h>

class Dex {
public:
    static void loadAndInvokeLoader(
            JNIEnv *env,
            const std::string &packageName,
            const void *dexFile, size_t dexFileLength,
            const void *propertiesFile, size_t propertiesLength,
            bool setTrusted, bool isDebuggable
    );
};
