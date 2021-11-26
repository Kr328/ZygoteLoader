package com.github.kr328.zloader.gradle.util

import org.gradle.api.Project
import java.io.File

val Project.intermediatesDir: File
    get() = buildDir.resolve("intermediates")