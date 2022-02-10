#include "entrypoint.h"

#include "delegate.h"
#include "logger.h"
#include "dex.h"
#include "scoped.h"
#include "process_utils.h"
#include "properties_utils.h"

#include <string>
#include <android/sharedmem.h>
#include <sys/mman.h>

void entrypoint(Delegate *delegate) {
    delegate->setModuleInfoResolver([delegate]() -> ModuleInfo * {
        auto info = new ModuleInfo();

        info->versionName = "";
        info->versionCode = 0;

        Resource *moduleProp = delegate->getResource(MODULE_PROP);

        PropertiesUtils::forEach(
                moduleProp->base,
                moduleProp->length,
                [info](auto key, auto value) {
                    if (key == "version") {
                        info->versionName = value;
                    } else if (key == "versionCode") {
                        info->versionCode = static_cast<int>(strtol(value.c_str(), nullptr, 10));
                    }
                }
        );

        return info;
    });

    delegate->setLoaderFactory([delegate](
            JNIEnv *env, std::string const &processName, bool isDebuggable
    ) -> Loader {
        if (!delegate->shouldEnableForPackage(processName)) {
            return [](JNIEnv *) {};
        }

        Resource *classesDex = delegate->getResource(CLASSES_DEX);
        Resource *moduleProp = delegate->getResource(MODULE_PROP);

        return [processName, isDebuggable, classesDex, moduleProp](JNIEnv *env) {
            Dex::loadAndInvokeLoader(
                    env,
                    processName,
                    classesDex->base, classesDex->length,
                    moduleProp->base, moduleProp->length,
                    processName != PACKAGE_NAME_SYSTEM_SERVER,
                    isDebuggable
            );
        };
    });
}
