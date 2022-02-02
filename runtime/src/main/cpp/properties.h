#pragma once

#include "chunk.h"

#include <string>
#include <functional>

class Properties {
public:
    using PropertyReceiver = std::function<void (std::string const &key, std::string const &value)>;

public:
    static void forEach(Chunk *data, const PropertyReceiver& block);
};
