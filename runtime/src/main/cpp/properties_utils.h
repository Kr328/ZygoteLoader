#pragma once

#include <string>
#include <functional>

class PropertiesUtils {
public:
    using PropertyReceiver = std::function<void (std::string const &key, std::string const &value)>;

public:
    static void forEach(const void *data, size_t length, const PropertyReceiver& block);
};
