#include "entrypoint.h"

#include "delegate.h"
#include "logger.h"
#include "properties.h"
#include "dex.h"

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

    std::unique_ptr<Properties> properties{Properties::load(propertiesFile)};
    std::string versionName = properties->get("version");
    std::string versionCode = properties->get("versionCode");
    std::string dataDirectory = properties->get("dataDirectory");
    if (dataDirectory.empty()) {
        Logger::e("Unknown module directory");

        return;
    }

    delegate->setModuleInfoResolver([versionCode, versionName]() -> ModuleInfo * {
        auto info = new ModuleInfo();

        info->versionName = versionName;
        info->versionCode = static_cast<int>(strtol(versionCode.c_str(), nullptr, 10));

        return info;
    });

    delegate->setLoaderFactory([delegate, dataDirectory, propertiesFile, dexFile](
            JNIEnv *env, std::string const &processName
    ) -> Loader {
        if (!delegate->isResourceExisted("packages/" + processName)) {
            if (!delegate->isResourceExisted(dataDirectory + "/packages/" + processName)) {
                return [](JNIEnv *) {};
            }
        }

        return [processName, propertiesFile, dexFile](JNIEnv *env) {
            std::string propertiesText = std::string(
                    static_cast<char *>(propertiesFile->getData()),
                    propertiesFile->getLength()
            );

            Dex::loadAndInvokeLoader(dexFile, env, processName, propertiesText);

            delete propertiesFile;
        };
    });
}