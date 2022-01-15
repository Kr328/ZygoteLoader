#pragma once

#include <jni.h>
#include <string>

namespace Dex {
    void loadAndInvokeLoader(
            JNIEnv *env,
            void *dex, int dexLength,
            const std::string &packageName,
            const std::string &properties
    );
}