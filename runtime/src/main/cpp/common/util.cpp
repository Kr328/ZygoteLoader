#include "util.h"

#include <random>
#include <climits>

std::string Util::resolvePackageName(JNIEnv *env, jstring niceName) {
    const char *_niceName = env->GetStringUTFChars(niceName, nullptr);

    std::string s = _niceName;
    size_t pos = s.find(':');
    if (pos != std::string::npos) {
        s = s.substr(0, pos);
    }

    env->ReleaseStringUTFChars(niceName, _niceName);

    return s;
}

std::string Util::randomFilename(int length) {
    static const char *table = "abcdefghijklmnopqrstuvwxyz";
    static int tableLength = strlen(table);

    std::random_device r;

    char *array = new char[length + 1];

    for (int i = 0; i < length; i++) {
        array[i] = table[r() % tableLength];
    }

    array[length] = 0;

    std::string result = array;

    delete[]array;

    return result;
}