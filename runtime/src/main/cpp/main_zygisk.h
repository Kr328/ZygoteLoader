#pragma once

#include "main.h"

#include "ext/zygisk.hpp"

#include <string.h>

class ZygoteLoaderModule : public zygisk::ModuleBase {
public:
    void onLoad(zygisk::Api *api, JNIEnv *env) override;
    void preAppSpecialize(zygisk::AppSpecializeArgs *args) override;
    void postAppSpecialize(const zygisk::AppSpecializeArgs *args) override;
    void preServerSpecialize(zygisk::ServerSpecializeArgs *args) override;
    void postServerSpecialize(const zygisk::ServerSpecializeArgs *args) override;

public:
    void fetchResources();
    void reset();
    bool shouldEnableForPackage(const char *packageName);
    void prepareFork();
    void tryLoadDex();

private:
    void initialize();
    bool isInitialized();
    bool isUseBinderInterceptors();

private:
    zygisk::Api *api = nullptr;
    JNIEnv *env = nullptr;

    Resource *moduleProp = nullptr;
    Resource *classesDex = nullptr;

    char *currentProcessName = nullptr;
    bool useBinderInterceptors = false;
};