#pragma once

#include <string>
#include <android/log.h>

#define TAG "ZygoteLoader"

class Logger {
public:
    static void i(std::string const &msg);
    static void e(std::string const &msg);
};