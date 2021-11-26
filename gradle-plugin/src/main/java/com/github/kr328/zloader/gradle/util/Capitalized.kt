package com.github.kr328.zloader.gradle.util

fun String.toCapitalized(): String {
    return replaceFirstChar { it.uppercase() }
}