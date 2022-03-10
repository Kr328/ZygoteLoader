package com.github.kr328.gradle.zygote.util

fun String.toCapitalized(): String {
    return replaceFirstChar { it.uppercase() }
}