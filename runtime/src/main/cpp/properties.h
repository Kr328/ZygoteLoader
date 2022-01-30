#pragma once

#include "chunk.h"

#include <unordered_map>
#include <string>

class Properties {
private:
    Properties(std::unordered_map<std::string, std::string> const &properties);

public:
    std::string get(std::string const &key) const;

public:
    static Properties *load(Chunk *chunk);

private:
    std::unordered_map<std::string, std::string> properties;
};
