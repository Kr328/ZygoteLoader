#pragma once

#include <functional>

#include "logger.h"

#ifdef DEBUG
#define TRACE_SCOPE(msg) Logger::d("ENTER[" msg "]"); Defer __trace{[](){Logger::d("EXIT[" msg "]");}}
#define DEBUG_LOG(msg) Logger::d(msg)
#else
#define TRACE_SCOPE(msg)
#define DEBUG_LOG(msg)
#endif

struct Defer {
    std::function<void ()> defer;

    inline Defer(std::function<void ()> defer) : defer(defer) {}
    inline ~Defer() { defer(); };
};