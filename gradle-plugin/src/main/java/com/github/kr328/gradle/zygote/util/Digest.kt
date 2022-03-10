package com.github.kr328.gradle.zygote.util

import java.security.MessageDigest

fun MessageDigest.digestHexString(bytes: ByteArray): String {
    return digest(bytes).joinToString("") { b ->
        String.format("%02x", b.toInt() and 0xff)
    }
}