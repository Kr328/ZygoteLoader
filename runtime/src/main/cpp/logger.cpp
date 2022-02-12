#include "logger.h"

void Logger::i(const std::string &msg) {
    __android_log_print(ANDROID_LOG_INFO, TAG, "%s", msg.c_str());
}

void Logger::d(const std::string &msg) {
    __android_log_print(ANDROID_LOG_DEBUG, TAG, "%s", msg.c_str());
}

void Logger::e(const std::string &msg) {
    __android_log_print(ANDROID_LOG_ERROR, TAG, "%s", msg.c_str());
}

void Logger::f(const std::string &msg) {
    __android_log_print(ANDROID_LOG_FATAL, TAG, "%s", msg.c_str());
    abort();
}
