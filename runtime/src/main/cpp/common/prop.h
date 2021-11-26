#pragma once

#include <string>

namespace Prop {
    void parse(const std::string &data);

    std::string getText();
    std::string getProp(const std::string &key);
}
