package com.github.kr328.zloader.gradle

abstract class ZygoteLoaderExtension {
    class Properties : HashMap<String, String>() {
        var id: String by this
        var name: String by this
        var author: String by this
        var description: String by this
        var entrypoint: String by this
        var archiveName: String by this

        val isValid: Boolean
            get() = getOrDefault("id", "").isNotBlank() &&
                    getOrDefault("entrypoint", "").isNotBlank()
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