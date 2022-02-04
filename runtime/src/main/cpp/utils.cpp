#include "utils.h"

#include "scoped.h"

#include <climits>
#include <fcntl.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/_system_properties.h>

int IOUtils::readFull(int fd, void *buffer, uint32_t size) {
    auto ptr = static_cast<uint8_t *>(buffer);
    auto remain = size;

    while (remain > 0) {
        int r = read(fd, ptr, remain);
        if (r < 0) {
            return r;
        }

        ptr += r;
        remain -= r;
    }

    return static_cast<int>(size);
}

int IOUtils::writeFull(int fd, const void *buffer, uint32_t size) {
    auto ptr = static_cast<const uint8_t *>(buffer);
    auto remain = size;

    while (remain > 0) {
        int r = write(fd, ptr, remain);
        if (r < 0) {
            return r;
        }

        ptr += r;
        remain -= r;
    }

    return static_cast<int>(size);
}

Chunk *IOUtils::readFile(const std::string &path) {
    ScopedFileDescriptor fd = open(path.c_str(), O_RDONLY);
    if (fd.fd < 0) {
        return nullptr;
    }

    struct stat stat{};
    if (fstat(fd.fd, &stat) < 0) {
        return nullptr;
    }

    auto result = new Chunk(stat.st_size);
    if (readFull(fd.fd, result->getData(), result->getLength()) < 0) {
        delete result;

        return nullptr;
    }

    return result;
}

int IOUtils::sendAll(int out, int in, uint32_t size) {
    auto remain = size;

    while (remain > 0) {
        int r = sendfile(out, in, nullptr, remain);
        if (r < 0) {
            return r;
        }

        remain -= r;
    }

    return static_cast<int>(size);
}

std::string IOUtils::resolveFdPath(int fd) {
    char fdPath[PATH_MAX];
    sprintf(fdPath, "/proc/self/fd/%d", fd);

    char path[PATH_MAX];
    int r = readlink(fdPath, path, PATH_MAX);
    if (r < 0) {
        return "";
    }

    path[r] = 0;

    return path;
}

std::string IOUtils::latestError() {
    return strerror(errno);
}

Chunk *SerialUtils::readChunk(int fd) {
    uint32_t size = 0;

    if (IOUtils::readFull(fd, &size, sizeof(uint32_t)) < 0) {
        return nullptr;
    }

    auto chunk = new Chunk(size);

    if (IOUtils::readFull(fd, chunk->getData(), size) < 0) {
        delete chunk;

        return nullptr;
    }

    return chunk;
}

bool SerialUtils::writeChunk(int fd, Chunk const *chunk) {
    uint32_t size = chunk->getLength();

    if (IOUtils::writeFull(fd, &size, sizeof(uint32_t)) < 0) {
        return false;
    }

    if (IOUtils::writeFull(fd, chunk->getData(), size) < 0) {
        return false;
    }

    return true;
}

std::string JNIUtils::resolvePackageName(JNIEnv *env, jstring niceName) {
    const char *_niceName = env->GetStringUTFChars(niceName, nullptr);

    std::string s = _niceName;
    size_t pos = s.find(':');
    if (pos != std::string::npos) {
        s = s.substr(0, pos);
    }

    env->ReleaseStringUTFChars(niceName, _niceName);

    return s;
}

bool SystemProperties::isDebuggable() {
    char buffer[16] = {0};

    __system_property_get("ro.debuggable", buffer);

    return buffer[0] == '1';
}

int SystemProperties::getSdkVersion() {
    char buffer[128] = {0};

    __system_property_get("ro.build.version.sdk", buffer);

    return strtol(buffer, nullptr, 10);
}
