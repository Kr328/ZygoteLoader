#pragma once

#include <string>
#include <android/log.h>
#include <cstdlib>

#define TAG "ZygoteLoader"

#ifdef DEBUG
#define fatal_assert(expr) if (!(expr)) Logger::f("!(" #expr ")")
#else
#define fatal_assert(expr) if (!(expr)) abort()
#endif

class Logger {
public:
    static void i(std::string const &msg);
    static void e(std::string const &msg);
    static void f(std::string const &msg);
};