#pragma once

#include "chunk.h"

#include <string>
#include <jni.h>

class Dex {
public:
    static bool loadAndInvokeLoader(
            Chunk *file,
            JNIEnv *env,
            std::string const &packageName,
            std::string const &properties,
            bool setTrusted, bool isDebuggable
    );
};