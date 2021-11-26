package com.github.kr328.zloader.gradle

abstract class ZygoteLoaderExtension {
    class Properties : HashMap<String, String>() {
        var id: String by withDefault { "" }
        var name: String by withDefault { "" }
        var author: String by withDefault { "" }
        var description: String by withDefault { "" }
        var entrypoint: String by withDefault { "" }

        val isValid: Boolean
            get() = id.isNotBlank() && entrypoint.isNotBlank()
    }

    val zygisk: Properties = Properties()
    val riru: Properties = Properties()
    val packages: MutableSet<String> = mutableSetOf()

    fun zygisk(block: Properties.() -> Unit) {
        zygisk.block()
    }

    fun riru(block: Properties.() -> Unit) {
        riru.block()
    }

    fun packages(vararg pkgs: String) {
        packages.addAll(pkgs)
    }
}