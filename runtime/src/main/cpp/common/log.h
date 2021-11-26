#pragma once

#include <string>

namespace Log {
    extern void setModuleName(const std::string &moduleName);
    extern void i(const std::string &msg);
    extern void e(const std::string &msg);
}