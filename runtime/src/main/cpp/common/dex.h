#pragma once

#include <jni.h>
#include <string>

namespace Dex {
    void loadAndInvokeLoader(
            JNIEnv *env,
            const std::string &dexPath,
            const std::string &packageName,
            const std::string &properties
    );
}