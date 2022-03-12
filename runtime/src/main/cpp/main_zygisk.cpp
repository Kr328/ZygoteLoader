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
    INITIALIZE, IS_INITIALIZED, GET_RESOURCES, SHOULD_ENABLE_FOR_PACKAGE
};

static void findDataDirectory(char *output, const char *key, const char *value) {
    if (strcmp(key, "dataDirectory") == 0) {
        strcpy(output, value);
    }
}

static void handleFileRequest(int client) {
    static pthread_mutex_t initializeLock = PTHREAD_MUTEX_INITIALIZER;
    static int moduleProp = -1;
    static int classesDex = -1;
    static int moduleDirectory = -1;
    static int dataDirectory = -1;

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

                char dataDirectoryPath[PATH_MAX] = {0};
                Resource *block = resource_map_fd(moduleProp);
                properties_for_each(
                        block->base, block->length,
                        dataDirectoryPath,
                        reinterpret_cast<properties_for_each_block>(&findDataDirectory)
                );
                resource_release(block);
                fatal_assert(strlen(dataDirectoryPath) > 0);

                dataDirectory = open(dataDirectoryPath, O_RDONLY | O_DIRECTORY);
                fatal_assert(dataDirectory >= 0);

                LOGD("Remote initialized: dataDirectory = %s", dataDirectoryPath);
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

    api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);
}

void ZygoteLoaderModule::preAppSpecialize(zygisk::AppSpecializeArgs *args) {
    process_get_package_name(env, args->nice_name, &currentProcessName);
    if (shouldEnableForPackage(currentProcessName)) {
        debuggable = (args->runtime_flags & ZYGOTE_DEBUG_ENABLE_JDWP) != 0;
        fetchResources();
    } else {
        free(currentProcessName);
        currentProcessName = nullptr;
    }
}

void ZygoteLoaderModule::postAppSpecialize(const zygisk::AppSpecializeArgs *args) {
    tryLoadDex(false);
}

void ZygoteLoaderModule::preServerSpecialize(zygisk::ServerSpecializeArgs *args) {
    if (shouldEnableForPackage(PACKAGE_NAME_SYSTEM_SERVER)) {
        currentProcessName = strdup(PACKAGE_NAME_SYSTEM_SERVER);
        fetchResources();
    }
}

void ZygoteLoaderModule::postServerSpecialize(const zygisk::ServerSpecializeArgs *args) {
    tryLoadDex(true);
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

void ZygoteLoaderModule::releaseResources() {
    resource_release(moduleProp);
    resource_release(classesDex);
}

void ZygoteLoaderModule::tryLoadDex(bool systemServer) {
    if (currentProcessName != nullptr && classesDex != nullptr && moduleProp != nullptr) {
        LOGD("Enable for package %s", currentProcessName);

        LOGD("Loading in %s: setTrusted = %d, debuggable = %d", currentProcessName, !systemServer, debuggable);

        dex_load_and_invoke(
                env, currentProcessName,
                classesDex->base, classesDex->length,
                moduleProp->base, moduleProp->length,
                !systemServer, debuggable
        );

        free(currentProcessName);
        releaseResources();
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

bool ZygoteLoaderModule::isInitialized() {
    int remote = api->connectCompanion();
    fatal_assert(remote >= 0);

    fatal_assert(serializer_write_int(remote, IS_INITIALIZED) > 0);

    int initialized = -1;
    fatal_assert(serializer_read_int(remote, &initialized) > 0);

    close(remote);

    return initialized != 0;
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

REGISTER_ZYGISK_MODULE(ZygoteLoaderModule)

REGISTER_ZYGISK_COMPANION(handleFileRequest)
