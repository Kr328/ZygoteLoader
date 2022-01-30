#include "delegate_zygisk.h"

#include "utils.h"
#include "logger.h"
#include "scoped.h"
#include "entrypoint.h"

#include "ext/zygisk.hpp"

#include <memory>
#include <jni.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

static void handleFileRequest(int client) {
    std::unique_ptr<Chunk> requestData{SerialUtils::readChunk(client)};
    if (requestData == nullptr) {
        Logger::e("Read file request: " + IOUtils::latestError());

        return;
    }
    if (requestData->getLength() < sizeof(FileRequest) + 1) {
        Logger::e("Read file request: invalid length");

        return;
    }
    if (static_cast<uint8_t *>(requestData->getData())[requestData->getLength() - 1] != 0) {
        Logger::e("Read file request: unexpected EOF");

        return;
    }

    auto request = static_cast<FileRequest *>(requestData->getData());
    errno = 0;

    switch (request->command) {
        case READ: {
            ScopedFileDescriptor fd = open(request->path, O_RDONLY);

            int32_t error = errno;
            IOUtils::writeFull(client, &error, sizeof(int32_t));

            if (fd.fd >= 0 && error == 0) {
                uint32_t size = 0;

                struct stat stat{};
                if (fstat(fd.fd, &stat) >= 0) {
                    size = stat.st_size;
                }

                IOUtils::writeFull(client, &size, sizeof(uint32_t));
                IOUtils::sendAll(client, fd.fd, size);
            }

            break;
        }
        case IS_EXIST: {
            uint32_t existed = access(request->path, F_OK) == 0;

            IOUtils::writeFull(client, &existed, sizeof(uint32_t));

            break;
        }
    }
}

static bool sendFileRequest(int remote, FileCommand command, std::string const &path) {
    std::unique_ptr<Chunk> requestData{new Chunk(sizeof(FileRequest) + path.size() + 1)};
    auto request = static_cast<FileRequest *>(requestData->getData());

    request->command = command;
    strncpy(request->path, path.c_str(), path.size() + 1);

    return SerialUtils::writeChunk(remote, requestData.get());
}

static std::string resolveModuleAbsolutePath(std::string const &moduleDir, std::string const &path) {
    if (!path.empty() && path[0] == '/') {
        return path;
    }

    return moduleDir + "/" + path;
}

void ZygoteLoaderModule::onLoad(zygisk::Api *_api, JNIEnv *_env) {
    api = _api;
    env = _env;

    ScopedFileDescriptor _moduleDir = _api->getModuleDir();
    moduleDirectory = IOUtils::resolveFdPath(_moduleDir);

    api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);

    entrypoint(this);
}

void ZygoteLoaderModule::preAppSpecialize(zygisk::AppSpecializeArgs *args) {
    currentProcessName = JNIUtils::resolvePackageName(env, args->nice_name);

    loader = factory(env, currentProcessName);
}

void ZygoteLoaderModule::postAppSpecialize(const zygisk::AppSpecializeArgs *args) {
    loader(env);
}

void ZygoteLoaderModule::preServerSpecialize(zygisk::ServerSpecializeArgs *args) {
    currentProcessName = PACKAGE_NAME_SYSTEM_SERVER;

    loader = factory(env, currentProcessName);
}

void ZygoteLoaderModule::postServerSpecialize(const zygisk::ServerSpecializeArgs *args) {
    loader(env);
}

Chunk *ZygoteLoaderModule::readResource(const std::string &path) {
    std::string absolutePath = resolveModuleAbsolutePath(moduleDirectory, path);

    ScopedFileDescriptor remote = api->connectCompanion();
    if (remote < 0) {
        Logger::e("Connect to companion: " + IOUtils::latestError());

        return nullptr;
    }

    if (!sendFileRequest(remote, READ, absolutePath)) {
        Logger::e("Send request to companion: " + IOUtils::latestError());
    }

    int32_t error = 0;
    if (IOUtils::readFull(remote, &error, sizeof(int32_t)) < 0) {
        Logger::e("Read error of " + absolutePath + ": " + IOUtils::latestError());

        return nullptr;
    }

    if (error != 0) {
        Logger::e("Open " + absolutePath + ": " + strerror(error));

        return nullptr;
    }

    Chunk *result = SerialUtils::readChunk(remote);
    if (result == nullptr) {
        Logger::e("Read " + absolutePath + ": " + IOUtils::latestError());
    }

    return result;
}

bool ZygoteLoaderModule::isResourceExisted(const std::string &path) {
    std::string absolutePath = resolveModuleAbsolutePath(moduleDirectory, path);

    ScopedFileDescriptor remote = api->connectCompanion();
    if (remote < 0) {
        Logger::e("Connect to companion: " + IOUtils::latestError());

        return false;
    }

    sendFileRequest(remote, IS_EXIST, absolutePath);

    uint32_t existed = 0;
    if (IOUtils::readFull(remote, &existed, sizeof(uint32_t)) < 0) {
        Logger::e("Read status of " + absolutePath + ": " + IOUtils::latestError());

        return false;
    }

    return existed != 0;
}

void ZygoteLoaderModule::setModuleInfoResolver(ModuleInfoResolver provider) {
    // unused
}

void ZygoteLoaderModule::setLoaderFactory(LoaderFactory _factory) {
    factory = _factory;
}

REGISTER_ZYGISK_MODULE(ZygoteLoaderModule)

REGISTER_ZYGISK_COMPANION(handleFileRequest)
