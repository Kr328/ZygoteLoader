#pragma once

#include "chunk.h"

#include <string>
#include <jni.h>

struct ModuleInfo {
    std::string versionName;
    int versionCode;
};

using Loader = std::function<void(JNIEnv *env)>;
using LoaderFactory = std::function<Loader(JNIEnv *env, std::string processName)>;
using ModuleInfoResolver = std::function<ModuleInfo *()>;

class Delegate {
public:
    virtual Chunk *readResource(std::string const &path) = 0;
    virtual bool isResourceExisted(std::string const &path) = 0;
    virtual void setModuleInfoResolver(ModuleInfoResolver provider) = 0;
    virtual void setLoaderFactory(LoaderFactory factory) = 0;
};
