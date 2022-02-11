#include "delegate_zygisk.h"

#include "logger.h"
#include "scoped.h"
#include "entrypoint.h"
#include "serial_utils.h"
#include "properties_utils.h"
#include "process_utils.h"

#include <cstdlib>
#include <jni.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <linux/ashmem.h>

#define ASHMEM_DEVICE "/dev/ashmem"

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
            pthread_mutex_lock(&initializeLock);

            if (moduleDirectory == -1) {
                fatal_assert(SerialUtils::readFileDescriptor(client, moduleDirectory) > 0);

                ScopedFileDescriptor modulePropFd = openat(moduleDirectory, "module.prop", O_RDONLY);
                fatal_assert(modulePropFd >= 0);

                ScopedFileDescriptor classesDexFd = openat(moduleDirectory, "classes.dex", O_RDONLY);
                fatal_assert(classesDexFd >= 0);

                struct stat modulePropStat{};
                fatal_assert(fstat(modulePropFd, &modulePropStat) >= 0);

                struct stat classesDexStat{};
                fatal_assert(fstat(classesDexFd, &classesDexStat) >= 0);

                moduleProp = open(ASHMEM_DEVICE, O_RDWR);
                fatal_assert(moduleProp >= 0);
                fatal_assert(ioctl(moduleProp, ASHMEM_SET_SIZE, modulePropStat.st_size) >= 0);

                classesDex = open(ASHMEM_DEVICE, O_RDWR);
                fatal_assert(classesDex >= 0);
                fatal_assert(ioctl(classesDex, ASHMEM_SET_SIZE, classesDexStat.st_size) >= 0);

                ScopedMemoryMapping modulePropBlock{
                        moduleProp,
                        static_cast<size_t>(modulePropStat.st_size),
                        PROT_READ | PROT_WRITE,
                };
                fatal_assert(modulePropBlock != nullptr);

                ScopedMemoryMapping classesDexBlock{
                        classesDex,
                        static_cast<size_t>(classesDexStat.st_size),
                        PROT_READ | PROT_WRITE,
                };
                fatal_assert(classesDexBlock != nullptr);

                fatal_assert(SerialUtils::readFull(modulePropFd, modulePropBlock, modulePropBlock.length) > 0);
                fatal_assert(SerialUtils::readFull(classesDexFd, classesDexBlock, classesDexBlock.length) > 0);

                fatal_assert(ioctl(moduleProp, ASHMEM_SET_PROT_MASK, PROT_READ) >= 0);
                fatal_assert(ioctl(classesDex, ASHMEM_SET_PROT_MASK, PROT_READ) >= 0);

                PropertiesUtils::forEach(modulePropBlock, modulePropBlock.length, [](auto key, auto value) {
                    if (key == "dataDirectory") {
                        dataDirectory = open(value.c_str(), O_RDONLY | O_DIRECTORY);
                    }
                });
                fatal_assert(dataDirectory >= 0);

                fatal_assert(SerialUtils::writeInt(client, 0) > 0);
            }

            pthread_mutex_unlock(&initializeLock);

            break;
        }
        case IS_INITIALIZED: {
            fatal_assert(SerialUtils::writeInt(client, moduleDirectory != -1 ? 1 : 0) > 0);

            break;
        }
        case GET_RESOURCE: {
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

    releaseResourcesCache();
}

void ZygoteLoaderModule::preServerSpecialize(zygisk::ServerSpecializeArgs *args) {
    currentProcessName = PACKAGE_NAME_SYSTEM_SERVER;

    loader = factory(env, currentProcessName, false);
}

void ZygoteLoaderModule::postServerSpecialize(const zygisk::ServerSpecializeArgs *args) {
    loader(env);

    releaseResourcesCache();
}

Resource *ZygoteLoaderModule::getResource(ResourceType type) {
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

    size_t length = ioctl(fd, ASHMEM_GET_SIZE);
    fatal_assert(length >= 0);

    const void *base = mmap(nullptr, length, PROT_READ, MAP_SHARED, fd, 0);
    fatal_assert(base != MAP_FAILED);

    *cache = new Resource(base, length);

    return *cache;
}

bool ZygoteLoaderModule::shouldEnableForPackage(const std::string &packageName) {
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

void ZygoteLoaderModule::releaseResourcesCache() {
    if (moduleProp != nullptr) {
        munmap(const_cast<void*>(moduleProp->base), moduleProp->length);

        delete moduleProp;

        moduleProp = nullptr;
    }
    if (classesDex != nullptr) {
        munmap(const_cast<void*>(classesDex->base), classesDex->length);

        delete classesDex;

        classesDex = nullptr;
    }
}

bool ZygoteLoaderModule::isInitialized() {
    ScopedFileDescriptor remote = api->connectCompanion();
    fatal_assert(remote >= 0);

    fatal_assert(SerialUtils::writeInt(remote, IS_INITIALIZED) > 0);

    int initialized = -1;
    fatal_assert(SerialUtils::readInt(remote, initialized) > 0);

    return initialized != 0;
}

void ZygoteLoaderModule::initialize() {
    ScopedFileDescriptor remote = api->connectCompanion();

    ScopedFileDescriptor moduleDir = api->getModuleDir();
    fatal_assert(moduleDir >= 0);

    fatal_assert(SerialUtils::writeInt(remote, INITIALIZE) > 0);
    fatal_assert(SerialUtils::writeFileDescriptor(remote, moduleDir) > 0);

    int initialized = -1;
    fatal_assert(SerialUtils::readInt(remote, initialized) > 0);
    fatal_assert(initialized == 0);
}

REGISTER_ZYGISK_MODULE(ZygoteLoaderModule)

REGISTER_ZYGISK_COMPANION(handleFileRequest)
