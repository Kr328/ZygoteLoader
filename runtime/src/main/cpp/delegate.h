#pragma once

#include <string>
#include <jni.h>

struct ModuleInfo {
    std::string versionName;
    int versionCode;
};

struct Resource {
    void const * const base;
    size_t const length;

    inline Resource(void const *base, size_t length) : base(base), length(length) {}
};

enum ResourceType : int {
    MODULE_PROP, CLASSES_DEX
};

using Loader = std::function<void(JNIEnv *env)>;
using LoaderFactory = std::function<Loader(JNIEnv *env, std::string processName, bool isDebuggable)>;
using ModuleInfoResolver = std::function<ModuleInfo *()>;

class Delegate {
public:
    virtual Resource *getResource(ResourceType type) = 0;
    virtual bool shouldEnableForPackage(std::string const &packageName) = 0;
    virtual void setModuleInfoResolver(ModuleInfoResolver provider) = 0;
    virtual void setLoaderFactory(LoaderFactory factory) = 0;
};
