#pragma once

#include "delegate.h"

#include <string>

class ZygoteLoaderDelegate : public Delegate {
public:
    ZygoteLoaderDelegate(std::string const &moduleDir);

public:
    void initialize();
    void purgeResourceCache();

public:
    void preAppSpecialize(JNIEnv *env, jstring niceName, jint runtimeFlags);
    void postAppSpecialize(JNIEnv *env);
    void preServerSpecialize(JNIEnv *env);
    void postServerSpecialize(JNIEnv *env);

public:
    Resource *getResource(ResourceType type) override;
    bool shouldEnableForPackage(const std::string &packageName) override;
    void setModuleInfoResolver(ModuleInfoResolver resolver) override;
    void setLoaderFactory(LoaderFactory factory) override;

public:
    ModuleInfo *resolveModuleInfo();

private:
    std::string moduleDirectory;
    std::string dataDirectory;
    std::string currentProcessName;

    Resource *moduleProp = nullptr;
    Resource *classesDex = nullptr;

    ModuleInfoResolver resolver = []() { return nullptr; };
    LoaderFactory factory = [](JNIEnv *, std::string const &, bool) { return [](JNIEnv *) {}; };
    Loader loader = [](JNIEnv *) {};
};