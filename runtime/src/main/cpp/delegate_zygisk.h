#pragma once

#include "delegate.h"
#include "scoped.h"

#include "ext/zygisk.hpp"

#include <string>

class ZygoteLoaderModule : public zygisk::ModuleBase, public Delegate {
public:
    void onLoad(zygisk::Api *api, JNIEnv *env) override;
    void preAppSpecialize(zygisk::AppSpecializeArgs *args) override;
    void postAppSpecialize(const zygisk::AppSpecializeArgs *args) override;
    void preServerSpecialize(zygisk::ServerSpecializeArgs *args) override;
    void postServerSpecialize(const zygisk::ServerSpecializeArgs *args) override;

public:
    Resource *getResource(ResourceType type) override;
    bool shouldEnableForPackage(const std::string &packageName) override;
    void setModuleInfoResolver(ModuleInfoResolver provider) override;
    void setLoaderFactory(LoaderFactory factory) override;

private:
    bool isInitialized();
    void initialize();
    void purgeResourceCache();

private:
    zygisk::Api *api = nullptr;
    JNIEnv *env = nullptr;

    Resource *moduleProp = nullptr;
    Resource *classesDex = nullptr;

    std::string currentProcessName;

    Loader loader = [](JNIEnv *) {};
    LoaderFactory factory = [](JNIEnv *, std::string const &, bool) { return [](JNIEnv *) {}; };
};