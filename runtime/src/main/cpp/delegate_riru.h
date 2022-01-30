#pragma once

#include "delegate.h"
#include "chunk.h"

#include <string>

class ZygoteLoaderDelegate : public Delegate {
public:
    ZygoteLoaderDelegate(std::string const &moduleDir);

public:
    void preAppSpecialize(JNIEnv *env, jstring niceName);
    void postAppSpecialize(JNIEnv *env);
    void preServerSpecialize(JNIEnv *env);
    void postServerSpecialize(JNIEnv *env);

public:
    Chunk *readResource(const std::string &path) override;
    bool isResourceExisted(const std::string &path) override;
    void setModuleInfoResolver(ModuleInfoResolver resolver) override;
    void setLoaderFactory(LoaderFactory factory) override;

public:
    ModuleInfo *resolveModuleInfo();

private:
    std::string moduleDirectory;
    std::string currentProcessName;

    ModuleInfoResolver resolver = []() { return nullptr; };
    LoaderFactory factory = [](JNIEnv *, std::string const &) { return [](JNIEnv *) {}; };
    Loader loader = [](JNIEnv *) {};
};