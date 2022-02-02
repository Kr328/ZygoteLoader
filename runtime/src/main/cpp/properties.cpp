#include "properties.h"

#include <utility>

static std::string strip(const std::string_view &input) {
    auto start_it = std::find_if_not(input.begin(), input.end(), std::isspace);
    auto end_it = std::find_if_not(input.rbegin(), input.rend(), std::isspace);
    return std::string(start_it, end_it.base());
}

void Properties::forEach(Chunk *data, PropertyReceiver const& block) {
    std::string_view text{static_cast<char *>(data->getData()),
                          static_cast<size_t>(data->getLength())};

    for (size_t start = 0, end; start != std::string::npos; start = end) {
        end = text.find_first_of('\n', start + 1);
        if (end != std::string::npos) {
            auto line = text.substr(start, end);
            if (size_t split = line.find('='); split != std::string::npos) {
                block(strip(line.substr(0, split)), strip(line.substr(split + 1)));
            }
        }
    }
}
