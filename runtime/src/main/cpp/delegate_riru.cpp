#include "delegate_riru.h"

#include "entrypoint.h"
#include "utils.h"

#include "riru.h"

#include <string>
#include <unistd.h>
#include <memory>

#define MIN_API_VERSION 26
#define TARGET_API_VERSION 26

static std::string resolveModuleAbsolutePath(std::string const &moduleDir, std::string const &path) {
    if (!path.empty() && path[0] == '/') {
        return path;
    }

    return moduleDir + "/" + path;
}

ZygoteLoaderDelegate::ZygoteLoaderDelegate(const std::string &moduleDir) {
    moduleDirectory = moduleDir;
}

Chunk *ZygoteLoaderDelegate::readResource(const std::string &path) {
    return IOUtils::readFile(resolveModuleAbsolutePath(moduleDirectory, path));
}

bool ZygoteLoaderDelegate::isResourceExisted(const std::string &path) {
    std::string absolutePath = resolveModuleAbsolutePath(moduleDirectory, path);

    return access(absolutePath.c_str(), F_OK) == 0;
}

void ZygoteLoaderDelegate::setModuleInfoResolver(ModuleInfoResolver _resolver) {
    resolver = _resolver;
}

void ZygoteLoaderDelegate::setLoaderFactory(LoaderFactory _factory) {
    factory = _factory;
}

void ZygoteLoaderDelegate::preAppSpecialize(JNIEnv *env, jstring niceName) {
    currentProcessName = JNIUtils::resolvePackageName(env, niceName);

    loader = factory(env, currentProcessName);
}

void ZygoteLoaderDelegate::postAppSpecialize(JNIEnv *env) {
    loader(env);
}

void ZygoteLoaderDelegate::preServerSpecialize(JNIEnv *env) {
    currentProcessName = PACKAGE_NAME_SYSTEM_SERVER;

    loader = factory(env, currentProcessName);
}

void ZygoteLoaderDelegate::postServerSpecialize(JNIEnv *env) {
    loader(env);
}

ModuleInfo *ZygoteLoaderDelegate::resolveModuleInfo() {
    return resolver();
}

static int *riruAllowUnload;
static bool skipNext;
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
    skipNext = true;

    if (niceName != nullptr) {
        delegate->preAppSpecialize(env, *niceName);

        skipNext = false;
    }
}

static void nativeForkAndSpecializePost(JNIEnv *env, jclass cls, jint res) {
    if (res == 0) {
        if (!skipNext) {
            delegate->postAppSpecialize(env);
        }

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
    skipNext = true;

    if (niceName != nullptr) {
        delegate->preAppSpecialize(env, *niceName);

        skipNext = false;
    }
}

static void nativeSpecializeAppProcessPost(JNIEnv *env, jclass clazz) {
    if (!skipNext) {
        delegate->postAppSpecialize(env);
    }

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

    entrypoint(_delegate);

    std::unique_ptr<ModuleInfo> info{_delegate->resolveModuleInfo()};
    if (info == nullptr) {
        delete _delegate;

        return nullptr;
    }

    delegate = _delegate;

    auto module = new RiruVersionedModuleInfo();
    module->moduleApiVersion = apiVersion;
    module->moduleInfo = RiruModuleInfo{
            .supportHide = 1,
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