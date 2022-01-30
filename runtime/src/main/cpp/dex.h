#pragma once

#include "chunk.h"

#include <string>
#include <jni.h>

class Dex {
public:
    static void loadAndInvokeLoader(
            Chunk *file,
            JNIEnv *env,
            std::string const &packageName,
            std::string const &properties
    );
};