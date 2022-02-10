#include "delegate_riru.h"

#include "entrypoint.h"
#include "scoped.h"
#include "logger.h"
#include "process_utils.h"
#include "properties_utils.h"

#include "riru.h"

#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define MIN_API_VERSION 26
#define TARGET_API_VERSION 26

#ifdef DEBUG
#define SUPPORT_HIDE 0
#else
#define SUPPORT_HIDE 1
#endif

ZygoteLoaderDelegate::ZygoteLoaderDelegate(const std::string &moduleDir) {
    moduleDirectory = moduleDir;
}

void ZygoteLoaderDelegate::initialize() {
    Resource *modulePropRes = getResource(MODULE_PROP);
    PropertiesUtils::forEach(modulePropRes->base, modulePropRes->length, [this](auto key, auto value) {
        if (key == "dataDirectory") {
            dataDirectory = value;
        }
    });
}

void ZygoteLoaderDelegate::releaseResourcesCache() {
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

Resource *ZygoteLoaderDelegate::getResource(ResourceType type) {
    Resource **cache;
    std::string path;
    switch (type) {
        case MODULE_PROP: {
            cache = &moduleProp;
            path = moduleDirectory + "/module.prop";
            break;
        }
        case CLASSES_DEX: {
            cache = &classesDex;
            path = moduleDirectory + "/classes.dex";
            break;
        }
        default: {
            abort();
        }
    }

    if (*cache != nullptr) {
        return *cache;
    }

    ScopedFileDescriptor fd = open(path.c_str(), O_RDONLY);
    fatal_assert(fd >= 0);

    struct stat s{};
    fatal_assert(fstat(fd, &s) >= 0);

    const void *base = mmap(nullptr, s.st_size, PROT_READ, MAP_SHARED, fd, 0);
    fatal_assert(base != MAP_FAILED);

    *cache = new Resource(base, s.st_size);

    return *cache;
}

bool ZygoteLoaderDelegate::shouldEnableForPackage(const std::string &packageName) {
    std::string path;

    path = moduleDirectory + "/packages/" + packageName;
    if (access(path.c_str(), F_OK) == 0) {
        return true;
    }

    path = dataDirectory + "/packages/" + packageName;
    if (access(path.c_str(), F_OK) == 0) {
        return true;
    }

    return false;
}

void ZygoteLoaderDelegate::setModuleInfoResolver(ModuleInfoResolver _resolver) {
    resolver = _resolver;
}

void ZygoteLoaderDelegate::setLoaderFactory(LoaderFactory _factory) {
    factory = _factory;
}

void ZygoteLoaderDelegate::preAppSpecialize(JNIEnv *env, jstring niceName, jint runtimeFlags) {
    currentProcessName = ProcessUtils::resolveProcessName(env, niceName);

    loader = factory(env, currentProcessName, (runtimeFlags & ZYGOTE_DEBUG_ENABLE_JDWP) != 0);
}

void ZygoteLoaderDelegate::postAppSpecialize(JNIEnv *env) {
    loader(env);
}

void ZygoteLoaderDelegate::preServerSpecialize(JNIEnv *env) {
    currentProcessName = PACKAGE_NAME_SYSTEM_SERVER;

    loader = factory(env, currentProcessName, false);
}

void ZygoteLoaderDelegate::postServerSpecialize(JNIEnv *env) {
    loader(env);
}

ModuleInfo *ZygoteLoaderDelegate::resolveModuleInfo() {
    return resolver();
}

static int *riruAllowUnload;
static ZygoteLoaderDelegate *delegate;

static void enableUnload() {
    if (riruAllowUnload) {
        *riruAllowUnload = 1;
    }
}

static int shouldSkipUid(int uid) {
    return 0;
}

static void nativeForkSystemServerPre(
        JNIEnv *env, jclass cls,
        uid_t *uid, gid_t *gid, jintArray *gids,
        jint *runtimeFlags, jobjectArray *rlimits,
        jlong *permittedCapabilities,
        jlong *effectiveCapabilities
) {
    delegate->preServerSpecialize(env);
}

static void nativeForkSystemServerPost(JNIEnv *env, jclass cls, jint res) {
    if (res == 0) {
        delegate->postServerSpecialize(env);

        delegate->releaseResourcesCache();

        enableUnload();
    }
}

static void nativeForkAndSpecializePre(
        JNIEnv *env, jclass cls, jint *uid, jint *gid, jintArray *gids, jint *runtimeFlags,
        jobjectArray *rlimits, jint *mountExternal, jstring *seInfo, jstring *niceName,
        jintArray *fdsToClose, jintArray *fdsToIgnore, jboolean *is_child_zygote,
        jstring *instructionSet, jstring *appDataDir, jboolean *isTopApp,
        jobjectArray *pkgDataInfoList,
        jobjectArray *whitelistedDataInfoList, jboolean *bindMountAppDataDirs,
        jboolean *bindMountAppStorageDirs
) {
    delegate->preAppSpecialize(env, *niceName, *runtimeFlags);
}

static void nativeForkAndSpecializePost(JNIEnv *env, jclass cls, jint res) {
    if (res == 0) {
        delegate->postAppSpecialize(env);

        delegate->releaseResourcesCache();

        enableUnload();
    }
}

static void nativeSpecializeAppProcessPre(
        JNIEnv *env, jclass cls, jint *uid, jint *gid, jintArray *gids, jint *runtimeFlags,
        jobjectArray *rlimits, jint *mountExternal, jstring *seInfo, jstring *niceName,
        jboolean *startChildZygote, jstring *instructionSet, jstring *appDataDir,
        jboolean *isTopApp, jobjectArray *pkgDataInfoList, jobjectArray *whitelistedDataInfoList,
        jboolean *bindMountAppDataDirs, jboolean *bindMountAppStorageDirs
) {
    delegate->preAppSpecialize(env, *niceName, *runtimeFlags);
}

static void nativeSpecializeAppProcessPost(JNIEnv *env, jclass clazz) {
    delegate->postAppSpecialize(env);

    delegate->releaseResourcesCache();

    enableUnload();
}

extern "C"
__attribute__((visibility("default")))
RiruVersionedModuleInfo *init(Riru *riru) {
    int apiVersion = riru->riruApiVersion;
    if (apiVersion < MIN_API_VERSION) {
        return nullptr;
    }
    if (apiVersion > TARGET_API_VERSION)
        apiVersion = TARGET_API_VERSION;

    riruAllowUnload = riru->allowUnload;

    auto _delegate = new ZygoteLoaderDelegate(riru->magiskModulePath);

    _delegate->initialize();

    entrypoint(_delegate);

    std::unique_ptr<ModuleInfo> info{_delegate->resolveModuleInfo()};

    delegate = _delegate;

    auto module = new RiruVersionedModuleInfo();
    module->moduleApiVersion = apiVersion;
    module->moduleInfo = RiruModuleInfo{
            .supportHide = SUPPORT_HIDE,
            .version = info->versionCode,
            .versionName = strdup(info->versionName.c_str()),
            .onModuleLoaded = nullptr,
            .shouldSkipUid = &shouldSkipUid,
            .forkAndSpecializePre = &nativeForkAndSpecializePre,
            .forkAndSpecializePost = &nativeForkAndSpecializePost,
            .forkSystemServerPre = &nativeForkSystemServerPre,
            .forkSystemServerPost = &nativeForkSystemServerPost,
            .specializeAppProcessPre = &nativeSpecializeAppProcessPre,
            .specializeAppProcessPost = &nativeSpecializeAppProcessPost,
    };

    return module;
}