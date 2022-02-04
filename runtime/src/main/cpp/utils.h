#pragma once

#include "chunk.h"

#include <jni.h>
#include <string>

#define PACKAGE_NAME_SYSTEM_SERVER ".android"

#define ZYGOTE_DEBUG_ENABLE_JDWP 1u

class IOUtils {
public:
    static int readFull(int fd, void *buffer, uint32_t size);
    static int writeFull(int fd, const void *buffer, uint32_t size);
    static int sendAll(int out, int in, uint32_t size);
    static Chunk *readFile(std::string const &path);
    static std::string resolveFdPath(int fd);
    static std::string latestError();
};

class SerialUtils {
public:
    static Chunk *readChunk(int fd);
    static bool writeChunk(int fd, Chunk const *chunk);
};

class JNIUtils {
public:
    static std::string resolvePackageName(JNIEnv *env, jstring niceName);
};

class SystemProperties {
public:
    static bool isDebuggable();
    static int getSdkVersion();
};