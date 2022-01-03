#include "path.h"

const static std::string dynamicConfigPath = "/data/misc/zygote-loader";
static std::string modulePath;
static std::string moduleId;
static std::string publicJar;

void Path::setModulePath(const std::string &path) {
    ::modulePath = path;
}

void Path::setModuleId(const std::string &id) {
    ::moduleId = id;
}

void Path::setPublicJar(const std::string &path) {
    ::publicJar = path;
}

std::string Path::moduleProp() {
    return ::modulePath + "/module.prop";
}

std::string Path::prebuiltJar() {
    return ::modulePath + "/classes.jar";
}

std::string Path::staticPackagesPath() {
    return ::modulePath + "/packages";
}

std::string Path::dynamicPackagesPath() {
    return ::dynamicConfigPath + "/" + ::moduleId;
}

std::string Path::publicJar() {
    return ::publicJar;
}