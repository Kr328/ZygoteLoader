#include "process_utils.h"

#include <climits>
#include <cstdint>
#include <unistd.h>

std::string ProcessUtils::resolveProcessName(JNIEnv *env, jstring niceName) {
    const char *_niceName = env->GetStringUTFChars(niceName, nullptr);

    std::string s = _niceName;
    size_t pos = s.find(':');
    if (pos != std::string::npos) {
        s = s.substr(0, pos);
    }

    env->ReleaseStringUTFChars(niceName, _niceName);

    return s;
}

