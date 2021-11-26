#include "prop.h"

#include <sstream>
#include <string>
#include <unordered_map>

static std::string propertiesText;
static std::unordered_map<std::string, std::string> properties;

static std::string strip(const std::string &input){
    auto start_it = input.begin();
    auto end_it = input.rbegin();
    while (std::isspace(*start_it))
        ++start_it;
    while (std::isspace(*end_it))
        ++end_it;
    return std::string(start_it, end_it.base());
}

void Prop::parse(const std::string &data) {
    propertiesText = data;

    std::istringstream ss{data};
    std::string line{};

    while (std::getline(ss, line)) {
        size_t split = line.find('=');
        if (split == std::string::npos) {
            continue;
        }

        properties[strip(line.substr(0, split))] = strip(line.substr(split + 1));
    }
}

std::string Prop::getText() {
    return propertiesText;
}

std::string Prop::getProp(const std::string &key) {
    return properties[key];
}
