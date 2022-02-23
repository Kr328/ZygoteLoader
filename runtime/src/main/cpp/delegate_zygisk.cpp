#include "delegate_zygisk.h"

#include "logger.h"
#include "scoped.h"
#include "entrypoint.h"
#include "serial_utils.h"
#include "properties_utils.h"
#include "process_utils.h"
#include "debug.h"

#include <cstdlib>
#include <jni.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <linux/ashmem.h>

enum FileCommand : int {
    INITIALIZE, IS_INITIALIZED, GET_RESOURCE, SHOULD_ENABLE_FOR_PACKAGE
};

static void handleFileRequest(int client) {
    static pthread_mutex_t initializeLock = PTHREAD_MUTEX_INITIALIZER;
    static int moduleProp = -1;
    static int classesDex = -1;
    static int moduleDirectory = -1;
    static int dataDirectory = -1;

    ScopedBlocking blocking{client};
    fatal_assert(blocking.setBlocking(true));

    int command = -1;
    fatal_assert(SerialUtils::readInt(client, command) > 0);

    switch (static_cast<FileCommand>(command)) {
        case INITIALIZE: {
            TRACE_SCOPE("Remote: initialize");

            pthread_mutex_lock(&initializeLock);

            if (moduleDirectory == -1) {
                fatal_assert(SerialUtils::readFileDescriptor(client, moduleDirectory) > 0);

                moduleProp = openat(moduleDirectory, "module.prop", O_RDONLY);
                fatal_assert(moduleProp >= 0);

                classesDex = openat(moduleDirectory, "classes.dex", O_RDONLY);
                fatal_assert(classesDex >= 0);

                /* dataDirectory */ {
                    struct stat stat{};
                    fatal_assert(fstat(moduleProp, &stat) >= 0);

                    void *block = mmap(
                            nullptr,
                            stat.st_size,
                            PROT_READ,
                            MAP_PRIVATE,
                            moduleProp,
                            0
                    );
                    fatal_assert(block != MAP_FAILED);

                    PropertiesUtils::forEach(block, stat.st_size, [](auto key, auto value) {
                        if (key == "dataDirectory") {
                            if (dataDirectory >= 0) {
                                close(dataDirectory);
                            }
                            dataDirectory = open(value.c_str(), O_RDONLY | O_DIRECTORY);
                        }
                    });
                    fatal_assert(dataDirectory >= 0);

                    munmap(block, stat.st_size);
                }
            }

            fatal_assert(SerialUtils::writeInt(client, 1) > 0);

            pthread_mutex_unlock(&initializeLock);

            break;
        }
        case IS_INITIALIZED: {
            TRACE_SCOPE("Remote: is_initialized");

            pthread_mutex_lock(&initializeLock);

            fatal_assert(SerialUtils::writeInt(client, moduleDirectory != -1 ? 1 : 0) > 0);

            pthread_mutex_unlock(&initializeLock);

            break;
        }
        case GET_RESOURCE: {
            TRACE_SCOPE("Remote: get_resource");

            int type = -1;
            fatal_assert(SerialUtils::readInt(client, type) > 0);

            int fd;
            switch(static_cast<ResourceType>(type)) {
                case MODULE_PROP: {
                    fd = moduleProp;
                    break;
                }
                case CLASSES_DEX: {
                    fd = classesDex;
                    break;
                }
                default: {
                    abort();
                }
            }

            fatal_assert(SerialUtils::writeFileDescriptor(client, fd) > 0);

            break;
        }
        case SHOULD_ENABLE_FOR_PACKAGE: {
            TRACE_SCOPE("Remote: should_enable_for_package");

            std::string path;
            fatal_assert(SerialUtils::readString(client, path) >= 0);
            path = "packages/" + path;

            if (faccessat(moduleDirectory, path.c_str(), F_OK, 0) != 0) {
                if (faccessat(dataDirectory, path.c_str(), F_OK, 0) != 0) {
                    fatal_assert(SerialUtils::writeInt(client, 0) > 0);

                    break;
                }
            }

            fatal_assert(SerialUtils::writeInt(client, 1) > 0);

            break;
        }
        default: {
            Logger::f("Unknown command: " + std::to_string(command));

            break;
        }
    }
}

void ZygoteLoaderModule::onLoad(zygisk::Api *_api, JNIEnv *_env) {
    api = _api;
    env = _env;

    if (!isInitialized()) {
        initialize();
    }

    api->setOption(zygisk::DLCLOSE_MODULE_LIBRARY);

    entrypoint(this);
}

void ZygoteLoaderModule::preAppSpecialize(zygisk::AppSpecializeArgs *args) {
    currentProcessName = ProcessUtils::resolveProcessName(env, args->nice_name);

    loader = factory(env, currentProcessName, (args->runtime_flags & ZYGOTE_DEBUG_ENABLE_JDWP) != 0);
}

void ZygoteLoaderModule::postAppSpecialize(const zygisk::AppSpecializeArgs *args) {
    loader(env);

    purgeResourceCache();
}

void ZygoteLoaderModule::preServerSpecialize(zygisk::ServerSpecializeArgs *args) {
    currentProcessName = PACKAGE_NAME_SYSTEM_SERVER;

    loader = factory(env, currentProcessName, false);
}

void ZygoteLoaderModule::postServerSpecialize(const zygisk::ServerSpecializeArgs *args) {
    loader(env);

    purgeResourceCache();
}

Resource *ZygoteLoaderModule::getResource(ResourceType type) {
    TRACE_SCOPE("Client: get_resource");

    Resource **cache;
    switch (type) {
        case MODULE_PROP: {
            cache = &moduleProp;
            break;
        }
        case CLASSES_DEX: {
            cache = &classesDex;
            break;
        }
        default: {
            abort();
        }
    }
    if (*cache != nullptr) {
        return *cache;
    }

    ScopedFileDescriptor remote = api->connectCompanion();
    fatal_assert(remote >= 0);

    fatal_assert(SerialUtils::writeInt(remote, GET_RESOURCE) > 0);
    fatal_assert(SerialUtils::writeInt(remote, type) > 0);

    int _fd = -1;
    fatal_assert(SerialUtils::readFileDescriptor(remote, _fd) > 0);
    fatal_assert(_fd >= 0);

    ScopedFileDescriptor fd = _fd;

    struct stat stat{};
    fatal_assert(fstat(fd, &stat) >= 0);

    const void *base = mmap(nullptr, stat.st_size, PROT_READ, MAP_SHARED, fd, 0);
    fatal_assert(base != MAP_FAILED);

    *cache = new Resource(base, stat.st_size);

    return *cache;
}

bool ZygoteLoaderModule::shouldEnableForPackage(const std::string &packageName) {
    TRACE_SCOPE("Client: should_enable_for_package");

    ScopedFileDescriptor remote = api->connectCompanion();
    fatal_assert(remote >= 0);

    fatal_assert(SerialUtils::writeInt(remote, SHOULD_ENABLE_FOR_PACKAGE) > 0);
    fatal_assert(SerialUtils::writeString(remote, packageName) > 0);

    int r = 0;
    fatal_assert(SerialUtils::readInt(remote, r) > 0);

    return r != 0;
}

void ZygoteLoaderModule::setModuleInfoResolver(ModuleInfoResolver resolver) {
    // unused
}

void ZygoteLoaderModule::setLoaderFactory(LoaderFactory _factory) {
    factory = _factory;
}

void ZygoteLoaderModule::purgeResourceCache() {
    auto release = [](Resource **resource) {
        if (*resource != nullptr) {
            munmap(const_cast<void*>((*resource)->base), (*resource)->length);

            delete *resource;

            *resource = nullptr;
        }
    };

    release(&moduleProp);
    release(&classesDex);
}

bool ZygoteLoaderModule::isInitialized() {
    TRACE_SCOPE("Client: is_initialized");

    ScopedFileDescriptor remote = api->connectCompanion();
    fatal_assert(remote >= 0);

    fatal_assert(SerialUtils::writeInt(remote, IS_INITIALIZED) > 0);

    int initialized = -1;
    fatal_assert(SerialUtils::readInt(remote, initialized) > 0);

    return initialized != 0;
}

void ZygoteLoaderModule::initialize() {
    TRACE_SCOPE("Client: initialize");

    ScopedFileDescriptor remote = api->connectCompanion();

    ScopedFileDescriptor moduleDir = api->getModuleDir();
    fatal_assert(moduleDir >= 0);

    fatal_assert(SerialUtils::writeInt(remote, INITIALIZE) > 0);
    fatal_assert(SerialUtils::writeFileDescriptor(remote, moduleDir) > 0);

    int initialized = -1;
    fatal_assert(SerialUtils::readInt(remote, initialized) > 0);
    fatal_assert(initialized == 1);
}

REGISTER_ZYGISK_MODULE(ZygoteLoaderModule)

REGISTER_ZYGISK_COMPANION(handleFileRequest)
