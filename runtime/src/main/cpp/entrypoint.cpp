#include "entrypoint.h"

#include "delegate.h"
#include "logger.h"
#include "properties.h"
#include "dex.h"
#include "utils.h"

#include <string>

void entrypoint(Delegate *delegate) {
    Chunk *propertiesFile = delegate->readResource("module.prop");
    if (propertiesFile == nullptr) {
        return;
    }

    Chunk *dexFile = delegate->readResource("classes.dex");
    if (dexFile == nullptr) {
        return;
    }

    std::string dataDirectory;
    Properties::forEach(propertiesFile, [&dataDirectory](auto key, auto value) {
        if (key == "dataDirectory") {
            dataDirectory = value;
        }
    });
    if (dataDirectory.empty()) {
        Logger::e("Unknown module data directory");

        return;
    }

    delegate->setModuleInfoResolver([propertiesFile]() -> ModuleInfo * {
        auto info = new ModuleInfo();

        info->versionName = "";
        info->versionCode = 0;

        Properties::forEach(propertiesFile, [info](auto key, auto value) {
           if (key == "version") {
               info->versionName = value;
           } else if (key == "versionCode") {
               info->versionCode = static_cast<int>(strtol(value.c_str(), nullptr, 10));
           }
        });

        return info;
    });

    delegate->setLoaderFactory([delegate, dataDirectory, propertiesFile, dexFile](
            JNIEnv *env, std::string const &processName, bool isDebuggable
    ) -> Loader {
        if (!delegate->isResourceExisted("packages/" + processName)) {
            if (!delegate->isResourceExisted(dataDirectory + "/packages/" + processName)) {
                return [](JNIEnv *) {};
            }
        }

        return [processName, isDebuggable, propertiesFile, dexFile](JNIEnv *env) {
            std::string propertiesText = std::string(
                    static_cast<char *>(propertiesFile->getData()),
                    propertiesFile->getLength()
            );

            Dex::loadAndInvokeLoader(
                    dexFile,
                    env,
                    processName,
                    propertiesText,
                    processName != PACKAGE_NAME_SYSTEM_SERVER,
                    isDebuggable
            );

            delete propertiesFile;
        };
    });
}