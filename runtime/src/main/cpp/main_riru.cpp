#include "main_riru.h"

#include "logger.h"
#include "properties.h"
#include "process.h"
#include "dex.h"
#include "main.h"

#include "riru.h"

#include <stddef.h>
#include <string.h>
#include <stdio.h>
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

static char *packageName;
static int debuggable;

static int shouldSkipUid(int uid) {
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

static void onProcessForkPre(JNIEnv *env, jstring niceName, jint runtimeFlags) {
    if (niceName == NULL) {
        packageName = strdup(PACKAGE_NAME_SYSTEM_SERVER);
    } else {
        process_get_package_name(env, niceName, &packageName);
    }

    if (shouldEnableForPackage(packageName)) {
        debuggable = (runtimeFlags & ZYGOTE_DEBUG_ENABLE_JDWP) != 0;
    } else {
        free(packageName);
        packageName = NULL;
    }
}

static void onProcessForkPost(JNIEnv *env, jint res, int systemServer) {
    if (res == 0) {
        if (riruAllowUnload) {
            *riruAllowUnload = 1;
        }

        if (packageName != NULL) {
            LOGD("Loading in %s: setTrusted = %d, debuggable = %d", packageName, !systemServer, debuggable);

            dex_load_and_invoke(
                    env,
                    packageName,
                    classesDex->base, classesDex->length,
                    moduleProp->base, moduleProp->length,
                    !systemServer, debuggable
            );
        }

        resource_release(classesDex);
        resource_release(moduleProp);
    }

    free(packageName);
    packageName = NULL;
}

static void nativeForkSystemServerPre(
        JNIEnv *env, jclass cls,
        uid_t *uid, gid_t *gid, jintArray *gids,
        jint *runtimeFlags, jobjectArray *rlimits,
        jlong *permittedCapabilities,
        jlong *effectiveCapabilities
) {
    onProcessForkPre(env, NULL, 0);
}

static void nativeForkSystemServerPost(JNIEnv *env, jclass cls, jint res) {
    onProcessForkPost(env, res, 1);
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
    onProcessForkPre(env, *niceName, *runtimeFlags);
}

static void nativeForkAndSpecializePost(JNIEnv *env, jclass cls, jint res) {
    onProcessForkPost(env, res, 0);
}

static void nativeSpecializeAppProcessPre(
        JNIEnv *env, jclass cls, jint *uid, jint *gid, jintArray *gids, jint *runtimeFlags,
        jobjectArray *rlimits, jint *mountExternal, jstring *seInfo, jstring *niceName,
        jboolean *startChildZygote, jstring *instructionSet, jstring *appDataDir,
        jboolean *isTopApp, jobjectArray *pkgDataInfoList, jobjectArray *whitelistedDataInfoList,
        jboolean *bindMountAppDataDirs, jboolean *bindMountAppStorageDirs
) {
    onProcessForkPre(env, *niceName, *runtimeFlags);
}

static void nativeSpecializeAppProcessPost(JNIEnv *env, jclass clazz) {
    onProcessForkPost(env, 0, 0);
}

static void contributeModule(struct Module *module, const char *key, const char *value) {
    if (strcmp("version", key) == 0) {
        module->versionName = strdup(value);
    } else if (strcmp("versionCode", key) == 0) {
        module->versionCode = strtol(value, NULL, 10);
    } else if (strcmp("dataDirectory", key) == 0) {
        module->dataDirectory = strdup(value);
    }
}

RiruVersionedModuleInfo *init(Riru *riru) {
    int apiVersion = riru->riruApiVersion;
    if (apiVersion < MIN_API_VERSION) {
        return NULL;
    }
    if (apiVersion > TARGET_API_VERSION)
        apiVersion = TARGET_API_VERSION;

    riruAllowUnload = riru->allowUnload;
    moduleDirectory = strdup(riru->magiskModulePath);

    LOGD("Initializing: moduleDirectory = %s, riruApiVersion = %d", moduleDirectory, riru->riruApiVersion);

    char path[PATH_MAX] = {0};
    sprintf(path, "%s/%s", moduleDirectory, "module.prop");
    moduleProp = resource_map_file(path);
    sprintf(path, "%s/%s", moduleDirectory, "classes.dex");
    classesDex = resource_map_file(path);

    struct Module module = {
            .dataDirectory = NULL,
            .versionName = NULL,
            .versionCode = -1,
    };
    properties_for_each(
            moduleProp->base, moduleProp->length,
            &module, reinterpret_cast<properties_for_each_block>(&contributeModule)
    );
    fatal_assert(module.versionName != NULL);
    fatal_assert(module.versionCode >= 0);
    fatal_assert(module.dataDirectory != NULL);

    dataDirectory = module.dataDirectory;

    RiruVersionedModuleInfo *result = new RiruVersionedModuleInfo();
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
