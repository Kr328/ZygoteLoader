#pragma once

#include "riru.h"

struct Module {
    const char *dataDirectory;
    const char *versionName;
    int versionCode;
};

extern "C" __attribute__((visibility("default"))) RiruVersionedModuleInfo *init(Riru *riru);
