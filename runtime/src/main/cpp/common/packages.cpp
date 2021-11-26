#include "packages.h"

#include "path.h"

#include <unistd.h>

bool Packages::shouldEnableFor(const std::string &packageName) {
    std::string path;

    path = Path::staticPackagesPath() + "/" + packageName;
    if (access(path.c_str(), F_OK) == 0) {
        return true;
    }

    path = Path::dynamicPackagesPath() + "/" + packageName;
    if (access(path.c_str(), F_OK) == 0) {
        return true;
    }

    return false;
}
