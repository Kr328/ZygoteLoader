#include "main_zygisk.h"

#include "logger.h"
#include "serializer.h"
#include "properties.h"
#include "process.h"
#include "logger.h"
#include "dex.h"

#include <stdlib.h>
#include <jni.h>
#include <fcntl.h>
#include <unistd.h>
#include <malloc.h>
#include <pthread.h>

enum FileCommand : int {
    INITIALIZE, IS_INITIALIZED, IS_USE_BINDER_INTERCEPTORS, GET_RESOURCES, SHOULD_ENABLE_FOR_PACKAGE
};

struct InitializeData {
    char dataDirectory[PATH_MAX] = {0};
    bool useBinderInterceptors = false;
};

static void extractInitializeData(InitializeData *data, const char *key, const char *value) {
    if (strcmp(key, "dataDirectory") == 0) {
        strcpy(data->dataDirectory, value);
    } else if (strcmp(key, "useBinderInterceptors") == 0) {
        data->useBinderInterceptors = (strcmp(value, "true") == 0);
    }
}

static void handleFileRequest(int client) {
    static pthread_mutex_t initializeLock = PTHREAD_MUTEX_INITIALIZER;
    static int moduleProp = -1;
    static int classesDex = -1;
    static int moduleDirectory = -1;
    static int dataDirectory = -1;
    static bool useBinderInterceptors = false;

    int command = -1;
    fatal_assert(serializer_read_int(client, &command) > 0);

    switch (static_cast<FileCommand>(command)) {
        case INITIALIZE: {
            pthread_mutex_lock(&initializeLock);

            if (moduleDirectory == -1) {
                LOGD("Remote initializing");

                fatal_assert(serializer_read_file_descriptor(client, &moduleDirectory) > 0);

                moduleProp = openat(moduleDirectory, "module.prop", O_RDONLY);
                fatal_assert(moduleProp >= 0);

                classesDex = openat(moduleDirectory, "classes.dex", O_RDONLY);
                fatal_assert(classesDex >= 0);

                InitializeData initializeData;
                Resource *block = resource_map_fd(moduleProp);
                properties_for_each(
                        block->base, block->length,
                        &initializeData,
                        reinterpret_cast<properties_for_each_block>(&extractInitializeData)
                );
                resource_release(block);
                fatal_assert(strlen(initializeData.dataDirectory) > 0);

                dataDirectory = open(initializeData.dataDirectory, O_RDONLY | O_DIRECTORY);
                fatal_assert(dataDirectory >= 0);

                useBinderInterceptors = initializeData.useBinderInterceptors;

                LOGD("Remote initialized: dataDirectory = %s", initializeData.dataDirectory);
            }

            fatal_assert(serializer_write_int(client, 1) > 0);

            pthread_mutex_unlock(&initializeLock);

            break;
        }
        case IS_INITIALIZED: {
            pthread_mutex_lock(&initializeLock);

            fatal_assert(serializer_write_int(client, moduleDirectory != -1 ? 1 : 0) > 0);

            pthread_mutex_unlock(&initializeLock);

            break;
        }
        case IS_USE_BINDER_INTERCEPTORS: {
            fatal_assert(serializer_write_int(client, useBinderInterceptors ? 1 : 0) > 0);

            break;
        }
        case GET_RESOURCES: {
            fatal_assert(serializer_write_file_descriptor(client, moduleProp) > 0);
            fatal_assert(serializer_write_file_descriptor(client, classesDex) > 0);

            break;
        }
        case SHOULD_ENABLE_FOR_PACKAGE: {
            char *packageName = nullptr;
            fatal_assert(serializer_read_string(client, &packageName) > 0);

            char path[PATH_MAX] = {0};
            sprintf(path, "packages/%s", packageName);
            free(packageName);

            if (faccessat(moduleDirectory, path, F_OK, 0) != 0) {
                if (faccessat(dataDirectory, path, F_OK, 0) != 0) {
                    fatal_assert(serializer_write_int(client, 0) > 0);

                    break;
                }
            }

            fatal_assert(serializer_write_int(client, 1) > 0);

            break;
        }
        default: {
            LOGD("Unknown command: %d", command);

            break;
        }
    }
}

void ZygoteLoaderModule::onLoad(zygisk::Api *_api, JNIEnv *_env) {
    api = _api;
    env = _env;

    if (!isInitialized()) {
        LOGD("Requesting initialize");

        initialize();
    }
}

void ZygoteLoaderModule::preAppSpecialize(zygisk::AppSpecializeArgs *args) {
    process_get_package_name(env, args->nice_name, &currentProcessName);

    prepareFork();
}

void ZygoteLoaderModule::postAppSpecialize(const zygisk::AppSpecializeArgs *args) {
    tryLoadDex();
}

void ZygoteLoaderModule::preServerSpecialize(zygisk::ServerSpecializeArgs *args) {
    currentProcessName = strdup(PACKAGE_NAME_SYSTEM_SERVER);

    prepareFork();
}

void ZygoteLoaderModule::postServerSpecialize(const zygisk::ServerSpecializeArgs *args) {
    tryLoadDex();
}

void ZygoteLoaderModule::fetchResources() {
    int remote = api->connectCompanion();
    fatal_assert(remote >= 0);

    fatal_assert(serializer_write_int(remote, GET_RESOURCES) > 0);

    int modulePropFd = -1;
    int classesDexFd = -1;
    fatal_assert(serializer_read_file_descriptor(remote, &modulePropFd) > 0);
    fatal_assert(serializer_read_file_descriptor(remote, &classesDexFd) > 0);

    moduleProp = resource_map_fd(modulePropFd);
    classesDex = resource_map_fd(classesDexFd);

    close(remote);
    close(modulePropFd);
    close(classesDexFd);
}

void ZygoteLoaderModule::reset() {
    useBinderInterceptors = false;

    free(currentProcessName);

    currentProcessName = nullptr;

    if (moduleProp != nullptr) {
        resource_release(moduleProp);
    }
    if (classesDex != nullptr) {
        resource_release(classesDex);
    }
}

void ZygoteLoaderModule::prepareFork() {
    if (shouldEnableForPackage(currentProcessName)) {
        fetchResources();

        useBinderInterceptors = isUseBinderInterceptors();
    } else {
        reset();
    }

    if (!useBinderInterceptors) {
        LOGD("Request dlclose module");

        api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
    }
}

void ZygoteLoaderModule::tryLoadDex() {
    if (currentProcessName != nullptr && classesDex != nullptr && moduleProp != nullptr) {
        LOGD("Loading in %s", currentProcessName);

        dex_load_and_invoke(
                env, currentProcessName,
                classesDex->base, classesDex->length,
                moduleProp->base, moduleProp->length,
                useBinderInterceptors
        );

        reset();
    }
}

bool ZygoteLoaderModule::shouldEnableForPackage(const char *packageName) {
    int remote = api->connectCompanion();
    fatal_assert(remote >= 0);

    fatal_assert(serializer_write_int(remote, SHOULD_ENABLE_FOR_PACKAGE) > 0);
    fatal_assert(serializer_write_string(remote, packageName) > 0);

    int r = 0;
    fatal_assert(serializer_read_int(remote, &r) > 0);

    LOGD("Enabled status for %s: %d", packageName, r);

    close(remote);

    return r != 0;
}

void ZygoteLoaderModule::initialize() {
    int remote = api->connectCompanion();

    int moduleDir = api->getModuleDir();
    fatal_assert(moduleDir >= 0);

    fatal_assert(serializer_write_int(remote, INITIALIZE) > 0);
    fatal_assert(serializer_write_file_descriptor(remote, moduleDir) > 0);

    int initialized = -1;
    fatal_assert(serializer_read_int(remote, &initialized) > 0);
    fatal_assert(initialized == 1);
}

bool ZygoteLoaderModule::isInitialized() {
    int remote = api->connectCompanion();
    fatal_assert(remote >= 0);

    fatal_assert(serializer_write_int(remote, IS_INITIALIZED) > 0);

    int initialized = -1;
    fatal_assert(serializer_read_int(remote, &initialized) > 0);

    close(remote);

    return initialized != 0;
}

bool ZygoteLoaderModule::isUseBinderInterceptors() {
    int remote = api->connectCompanion();
    fatal_assert(remote >= 0);

    fatal_assert(serializer_write_int(remote, IS_USE_BINDER_INTERCEPTORS) > 0);

    int enabled = 0;
    fatal_assert(serializer_read_int(remote, &enabled) > 0);

    close(remote);

    return enabled != 0;
}


REGISTER_ZYGISK_MODULE(ZygoteLoaderModule)

REGISTER_ZYGISK_COMPANION(handleFileRequest)
