#pragma once

#include "delegate.h"

#include "ext/zygisk.hpp"

#include <string>

enum FileCommand : uint8_t {
    READ, IS_EXIST
};

struct FileRequest {
    FileCommand command;
    char path[];
};

class ZygoteLoaderModule : public zygisk::ModuleBase, public Delegate {
public:
    void onLoad(zygisk::Api *api, JNIEnv *env) override;
    void preAppSpecialize(zygisk::AppSpecializeArgs *args) override;
    void postAppSpecialize(const zygisk::AppSpecializeArgs *args) override;
    void preServerSpecialize(zygisk::ServerSpecializeArgs *args) override;
    void postServerSpecialize(const zygisk::ServerSpecializeArgs *args) override;

public:
    Chunk *readResource(const std::string &path) override;
    bool isResourceExisted(const std::string &path) override;
    void setModuleInfoResolver(ModuleInfoResolver provider) override;
    void setLoaderFactory(LoaderFactory factory) override;

private:
    zygisk::Api *api = nullptr;
    JNIEnv *env = nullptr;

    std::string moduleDirectory;
    std::string currentProcessName;

    Loader loader = [](JNIEnv *) {};
    LoaderFactory factory = [](JNIEnv *, std::string const &) { return [](JNIEnv *) {}; };
};