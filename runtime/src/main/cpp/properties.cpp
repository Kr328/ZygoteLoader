#include "properties.h"

#include <utility>

static std::string strip(const std::string_view &input) {
    auto start_it = std::find_if_not(input.begin(), input.end(), std::isspace);
    auto end_it = std::find_if_not(input.rbegin(), input.rend(), std::isspace);
    return std::string(start_it, end_it.base());
}

Properties::Properties(std::unordered_map<std::string, std::string> properties) :
        properties(std::move(properties)) {
}

std::string Properties::get(const std::string &key) const {
    if (auto value = properties.find(key); value == properties.end()) {
        return "";
    } else {
        return value->second;
    }
}

Properties *Properties::load(Chunk *chunk) {
    std::string_view text{static_cast<char *>(chunk->getData()),
                          static_cast<size_t>(chunk->getLength())};

    std::unordered_map<std::string, std::string> properties;

    for (size_t start = 0, end; start != std::string::npos; start = end) {
        end = text.find_first_of('\n', start + 1);
        if (end != std::string::npos) {
            auto line = text.substr(start, end);
            if (size_t split = line.find('='); split != std::string::npos) {
                properties[strip(line.substr(0, split))] = strip(line.substr(split + 1));
            }
        }
    }
    return new Properties(std::move(properties));
}
