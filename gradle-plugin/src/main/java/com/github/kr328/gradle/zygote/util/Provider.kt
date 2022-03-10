package com.github.kr328.gradle.zygote.util

import org.gradle.api.file.Directory
import org.gradle.api.provider.Provider
import java.io.File

fun Provider<File>.resolve(path: String): Provider<File> {
    return map { it.resolve(path) }
}

fun Provider<Directory>.asFile(): Provider<File> {
    return map { it.asFile }
}
