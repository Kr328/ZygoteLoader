#include "log.h"

#include <android/log.h>

static std::string tag = "ZygoteLoader[Native]";

void Log::setModuleName(const std::string &moduleName) {
    tag = moduleName;
}

void Log::i(const std::string &msg) {
    __android_log_print(ANDROID_LOG_INFO, tag.c_str(), "%s", msg.c_str());
}

void Log::e(const std::string &msg) {
    __android_log_print(ANDROID_LOG_ERROR, tag.c_str(), "%s", msg.c_str());
}
