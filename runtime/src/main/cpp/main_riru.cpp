#include "main_riru.h"

#include "logger.h"
#include "properties.h"
#include "process.h"
#include "dex.h"
#include "main.h"

#include "riru.h"

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define MIN_API_VERSION 26
#define TARGET_API_VERSION 26

#ifdef DEBUG
#define SUPPORT_HIDE 0
#else
#define SUPPORT_HIDE 1
#endif

static struct Resource *moduleProp;
static struct Resource *classesDex;

static int *riruAllowUnload;

static const char *moduleDirectory;
static const char *dataDirectory;
static int useBinderInterceptors;

static char *packageName;

static int shouldSkipUid(_unused int uid) {
    return 0;
}

static int shouldEnableForPackage(const char *pkg) {
    char path[PATH_MAX] = {0};

    sprintf(path, "%s/packages/%s", moduleDirectory, pkg);
    if (access(path, F_OK) == 0) {
        LOGD("Enabled for package %s", pkg);

        return 1;
    }

    sprintf(path, "%s/packages/%s", dataDirectory, pkg);
    if (access(path, F_OK) == 0) {
        LOGD("Enabled for package %s", pkg);

        return 1;
    }

    LOGD("Ignore package %s", pkg);

    return 0;
}

static void onProcessForkPre(JNIEnv *env, jstring niceName) {
    if (niceName == nullptr) {
        packageName = strdup(PACKAGE_NAME_SYSTEM_SERVER);
    } else {
        process_get_package_name(env, niceName, &packageName);
    }

    if (!shouldEnableForPackage(packageName)) {
        free(packageName);
        packageName = nullptr;
    }
}

static void onProcessForkPost(JNIEnv *env, jint res) {
    if (res == 0) {
        if (riruAllowUnload) {
            *riruAllowUnload = !useBinderInterceptors;
        }

        if (packageName != nullptr) {
            LOGD("Loading in %s", packageName);

            dex_load_and_invoke(
                    env,
                    packageName,
                    classesDex->base, classesDex->length,
                    moduleProp->base, moduleProp->length,
                    useBinderInterceptors
            );
        }

        resource_release(classesDex);
        resource_release(moduleProp);
    }

    free(packageName);
    packageName = nullptr;
}

static void nativeForkSystemServerPre(
        JNIEnv *env, _unused jclass cls,
        _unused uid_t *uid, _unused gid_t *gid, _unused jintArray *gids,
        _unused jint *runtimeFlags, _unused jobjectArray *rlimits,
        _unused jlong *permittedCapabilities,
        _unused jlong *effectiveCapabilities
) {
    onProcessForkPre(env, nullptr);
}

static void nativeForkSystemServerPost(JNIEnv *env, _unused jclass cls, jint res) {
    onProcessForkPost(env, res);
}

static void nativeForkAndSpecializePre(
        JNIEnv *env,
        _unused jclass cls,
        _unused jint *uid,
        _unused jint *gid,
        _unused jintArray *gids,
        _unused jint *runtimeFlags,
        _unused jobjectArray *rlimits,
        _unused jint *mountExternal,
        _unused jstring *seInfo,
        jstring *niceName,
        _unused jintArray *fdsToClose,
        _unused jintArray *fdsToIgnore,
        _unused jboolean *is_child_zygote,
        _unused jstring *instructionSet,
        _unused jstring *appDataDir,
        _unused jboolean *isTopApp,
        _unused jobjectArray *pkgDataInfoList,
        _unused jobjectArray *whitelistedDataInfoList,
        _unused jboolean *bindMountAppDataDirs,
        _unused jboolean *bindMountAppStorageDirs
) {
    onProcessForkPre(env, *niceName);
}

static void nativeForkAndSpecializePost(JNIEnv *env, _unused jclass cls, jint res) {
    onProcessForkPost(env, res);
}

static void nativeSpecializeAppProcessPre(
        JNIEnv *env,
        _unused jclass cls,
        _unused jint *uid,
        _unused jint *gid,
        _unused jintArray *gids,
        _unused jint *runtimeFlags,
        _unused jobjectArray *rlimits,
        _unused jint *mountExternal,
        _unused jstring *seInfo,
        jstring *niceName,
        _unused jboolean *startChildZygote,
        _unused jstring *instructionSet,
        _unused jstring *appDataDir,
        _unused jboolean *isTopApp,
        _unused jobjectArray *pkgDataInfoList,
        _unused jobjectArray *whitelistedDataInfoList,
        _unused jboolean *bindMountAppDataDirs,
        _unused jboolean *bindMountAppStorageDirs
) {
    onProcessForkPre(env, *niceName);
}

static void nativeSpecializeAppProcessPost(JNIEnv *env, _unused jclass clazz) {
    onProcessForkPost(env, 0);
}

static void scanModule(struct Module *module, const char *key, const char *value) {
    if (strcmp("version", key) == 0) {
        module->versionName = strdup(value);
    } else if (strcmp("versionCode", key) == 0) {
        module->versionCode = strtol(value, nullptr, 10);
    } else if (strcmp("dataDirectory", key) == 0) {
        module->dataDirectory = strdup(value);
    } else if (strcmp("useBinderInterceptors", key) == 0) {
        module->useBinderInterceptors = (strcmp("true", value) == 0);
    }
}

RiruVersionedModuleInfo *init(Riru *riru) {
    int apiVersion = riru->riruApiVersion;
    if (apiVersion < MIN_API_VERSION) {
        return nullptr;
    } else if (apiVersion > TARGET_API_VERSION) {
        apiVersion = TARGET_API_VERSION;
    }

    riruAllowUnload = riru->allowUnload;
    moduleDirectory = strdup(riru->magiskModulePath);

    LOGD("Initializing: moduleDirectory = %s, riruApiVersion = %d",
         moduleDirectory, riru->riruApiVersion);

    char path[PATH_MAX] = {0};
    sprintf(path, "%s/%s", moduleDirectory, "module.prop");
    moduleProp = resource_map_file(path);
    sprintf(path, "%s/%s", moduleDirectory, "classes.dex");
    classesDex = resource_map_file(path);

    struct Module module = {
            .dataDirectory = nullptr,
            .versionName = nullptr,
            .versionCode = -1,
            .useBinderInterceptors = 0,
    };
    properties_for_each(
            moduleProp->base, moduleProp->length,
            &module, reinterpret_cast<properties_for_each_block>(&scanModule)
    );
    fatal_assert(module.versionName != nullptr);
    fatal_assert(module.versionCode >= 0);
    fatal_assert(module.dataDirectory != nullptr);

    dataDirectory = module.dataDirectory;
    useBinderInterceptors = module.useBinderInterceptors;

    auto result = new RiruVersionedModuleInfo();
    result->moduleApiVersion = apiVersion;
    result->moduleInfo = RiruModuleInfo{
            .supportHide = SUPPORT_HIDE,
            .version = module.versionCode,
            .versionName = module.versionName,
            .onModuleLoaded = NULL,
            .shouldSkipUid = &shouldSkipUid,
            .forkAndSpecializePre = &nativeForkAndSpecializePre,
            .forkAndSpecializePost = &nativeForkAndSpecializePost,
            .forkSystemServerPre = &nativeForkSystemServerPre,
            .forkSystemServerPost = &nativeForkSystemServerPost,
            .specializeAppProcessPre = &nativeSpecializeAppProcessPre,
            .specializeAppProcessPost = &nativeSpecializeAppProcessPost,
    };

    return result;
}
