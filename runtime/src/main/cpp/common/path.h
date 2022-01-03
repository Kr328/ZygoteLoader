#pragma once

#include <string>

namespace Path {
    void setModulePath(const std::string &path);
    void setModuleId(const std::string &id);
    void setPublicJar(const std::string &path);

    std::string moduleProp();
    std::string prebuiltJar();
    std::string publicJar();
    std::string staticPackagesPath();
    std::string dynamicPackagesPath();
}