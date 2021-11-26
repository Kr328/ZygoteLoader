#include "path.h"

const static std::string dynamicConfigPath = "/data/misc/zygote-loader";
static std::string modulePath;
static std::string moduleId;

void Path::setModulePath(const std::string &path) {
    modulePath = path;
}

void Path::setModuleId(const std::string &id) {
    moduleId = id;
}

std::string Path::moduleProp() {
    return modulePath + "/module.prop";
}

std::string Path::classesDex() {
    return modulePath + "/classes.dex";
}

std::string Path::staticPackagesPath() {
    return modulePath + "/packages";
}

std::string Path::dynamicPackagesPath() {
    return dynamicConfigPath + "/" + moduleId;
}

