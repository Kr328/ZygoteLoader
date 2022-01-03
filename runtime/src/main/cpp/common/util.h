#pragma once

#include <string>
#include <jni.h>

namespace Util {
    std::string resolvePackageName(JNIEnv *env, jstring niceName);
    std::string randomFilename(int length);
}