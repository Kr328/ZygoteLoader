#include "io.h"
#include "log.h"
#include "riru.h"
#include "prop.h"
#include "dex.h"
#include "path.h"
#include "util.h"
#include "packages.h"

#include <fstream>
#include <string>
#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <sys/stat.h>

#define MIN_API_VERSION 26
#define TARGET_API_VERSION 26

#define PACKAGE_SERVER_SERVER ".android"

static int *riruAllowUnload;
static std::string packageName;

static void enableUnload() {
    if (riruAllowUnload) {
        *riruAllowUnload = 1;
    }
}

static void onModuleLoaded() {
    std::string filename;

    do {
        filename = "/dev/" + Util::randomFilename(32);
    } while (access(filename.c_str(), F_OK) == 0);

    std::ifstream input = std::ifstream(Path::prebuiltJar(), std::ios::binary);
    std::ofstream output = std::ofstream(filename, std::ios::binary);

    if (input && output) {
        output << input.rdbuf();

        chmod(filename.c_str(), 0644);

        Path::setPublicJar(filename);
    } else {
        Log::e(std::string("Copy jar file failed: ") + strerror(errno));
    }

    input.close();
    output.close();
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
    if (!Path::publicJar().empty()) {
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
                    Path::publicJar(),
                    packageName,
                    Prop::getText()
            );
        }

        enableUnload();
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
    if (!Path::publicJar().empty()) {
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
                    Path::publicJar(),
                    packageName,
                    Prop::getText()
            );
        }

        enableUnload();
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
    if (!Path::publicJar().empty()) {
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
                Path::publicJar(),
                packageName,
                Prop::getText()
        );
    }

    enableUnload();

    packageName.clear();
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

    Path::setModulePath(riru->magiskModulePath);

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
