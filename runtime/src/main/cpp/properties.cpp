#include "properties.h"

#include <utility>
#include <sstream>

static std::string strip(const std::string &input) {
    auto start_it = input.begin();
    auto end_it = input.rbegin();
    while (std::isspace(*start_it))
        ++start_it;
    while (std::isspace(*end_it))
        ++end_it;
    return std::string(start_it, end_it.base());
}

Properties::Properties(std::unordered_map<std::string, std::string> const &properties) {
    this->properties = properties;
}

std::string Properties::get(const std::string &key) const {
    auto value = properties.find(key);
    if (value == properties.end()) {
        return "";
    }
    return value->second;
}

Properties *Properties::load(Chunk *chunk) {
    std::string text = std::string(static_cast<char *>(chunk->getData()), chunk->getLength());
    std::istringstream ss{text};

    std::unordered_map<std::string, std::string> properties;
    std::string line;

    while (std::getline(ss, line)) {
        size_t split = line.find('=');
        if (split == std::string::npos) {
            continue;
        }

        properties[strip(line.substr(0, split))] = strip(line.substr(split + 1));
    }

    return new Properties(properties);
}
