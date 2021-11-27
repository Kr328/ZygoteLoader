#include "io.h"
#include "log.h"
#include "riru.h"
#include "prop.h"
#include "dex.h"
#include "path.h"
#include "util.h"
#include "packages.h"

#include <string>
#include <cstring>
#include <cerrno>

#define MIN_API_VERSION 26
#define TARGET_API_VERSION 26

#define PACKAGE_SERVER_SERVER ".android"

static Riru *riru;
static void *classes;
static int classesLength;
static std::string packageName;

static void onModuleLoaded() {
    classes = IO::readFile(Path::classesDex(), classesLength);
    if (classes == nullptr) {
        Log::e(std::string("Load classes.dex failed: ") + strerror(errno));
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
    if (classes != nullptr) {
        if (Packages::shouldEnableFor(PACKAGE_SERVER_SERVER)) {
            packageName = PACKAGE_SERVER_SERVER;
        }
    }
}

static void nativeForkSystemServerPost(JNIEnv *env, jclass cls, jint res) {
    if (res == 0) {
        if (!packageName.empty()) {
            Log::i("Load on " + packageName);

            Dex::loadAndInvokeLoader(
                    env,
                    classes, classesLength,
                    packageName,
                    Prop::getText()
            );
        }

        *riru->allowUnload = 1;
    }

    packageName.clear();
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
    if (classes != nullptr) {
        packageName = Util::resolvePackageName(env, *niceName);
        if (!packageName.empty()) {
            if (!Packages::shouldEnableFor(packageName)) {
                packageName.clear();
            }
        }
    }
}

static void nativeForkAndSpecializePost(JNIEnv *env, jclass cls, jint res) {
    if (res == 0) {
        if (!packageName.empty()) {
            Log::i("Load on " + packageName);

            Dex::loadAndInvokeLoader(
                    env,
                    classes, classesLength,
                    packageName,
                    Prop::getText()
            );
        }

        *riru->allowUnload = 1;
    }

    packageName.clear();
}

static void nativeSpecializeAppProcessPre(
        JNIEnv *env, jclass cls, jint *uid, jint *gid, jintArray *gids, jint *runtimeFlags,
        jobjectArray *rlimits, jint *mountExternal, jstring *seInfo, jstring *niceName,
        jboolean *startChildZygote, jstring *instructionSet, jstring *appDataDir,
        jboolean *isTopApp, jobjectArray *pkgDataInfoList, jobjectArray *whitelistedDataInfoList,
        jboolean *bindMountAppDataDirs, jboolean *bindMountAppStorageDirs
) {
    if (classes != nullptr) {
        packageName = Util::resolvePackageName(env, *niceName);
        if (!packageName.empty()) {
            if (!Packages::shouldEnableFor(packageName)) {
                packageName.clear();
            }
        }
    }
}

static void nativeSpecializeAppProcessPost(JNIEnv *env, jclass clazz) {
    if (!packageName.empty()) {
        Log::i("Load on " + packageName);

        Dex::loadAndInvokeLoader(
                env,
                classes, classesLength,
                packageName,
                Prop::getText()
        );
    }

    *riru->allowUnload = 1;

    packageName.clear();
}

extern "C"
__attribute__((visibility("default")))
RiruVersionedModuleInfo *init(Riru *_riru) {
    riru = _riru;

    int apiVersion = _riru->riruApiVersion;
    if (apiVersion < MIN_API_VERSION) {
        return nullptr;
    }
    if (apiVersion > TARGET_API_VERSION)
        apiVersion = TARGET_API_VERSION;

    Path::setModulePath(_riru->magiskModulePath);

    int size = 0;
    uint8_t *content = IO::readFile(Path::moduleProp(), size);
    if (content == nullptr) {
        Log::e(std::string("Read prop failed: ") + strerror(errno));

        return nullptr;
    }

    Prop::parse(std::string(reinterpret_cast<char *>(content), size));
    delete[] content;

    std::string moduleId = Prop::getProp("id");
    std::string moduleName = Prop::getProp("name");
    std::string versionCode = Prop::getProp("versionCode");
    std::string versionName = Prop::getProp("version");

    if (moduleId.empty() || moduleName.empty() || versionCode.empty() || versionName.empty()) {
        Log::e("Load moduleId/versionCode failed");

        return nullptr;
    }

    Path::setModuleId(moduleId);
    Log::setModuleName(moduleName);

    auto module = new RiruVersionedModuleInfo();
    module->moduleApiVersion = apiVersion;
    module->moduleInfo = RiruModuleInfo{
            .supportHide = 1,
            .version = static_cast<int>(strtol(versionCode.c_str(), nullptr, 10)),
            .versionName = strdup(versionName.c_str()),
            .onModuleLoaded = &onModuleLoaded,
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
