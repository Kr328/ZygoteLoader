package com.github.kr328.gradle.zygote

import org.gradle.api.Action

abstract class ZygoteLoaderExtension {
    class Properties : HashMap<String, String>() {
        var id: String by this
        var name: String by this
        var author: String by this
        var description: String by this
        var entrypoint: String by this
        var archiveName: String by this
        var updateJson: String by this
    }

    val zygisk: Properties = Properties()
    val riru: Properties = Properties()
    val packages: MutableSet<String> = mutableSetOf()

    fun zygisk(block: Properties.() -> Unit) {
        zygisk.block()
    }

    fun zygisk(block: Action<Properties>) {
        block.execute(zygisk)
    }

    fun riru(block: Properties.() -> Unit) {
        riru.block()
    }

    fun riru(block: Action<Properties>) {
        block.execute(riru)
    }

    fun all(block: Properties.() -> Unit) {
        zygisk(block)
        riru(block)
    }

    fun all(block: Action<Properties>) {
        zygisk(block)
        riru(block)
    }

    fun packages(vararg pkgs: String) {
        packages.addAll(pkgs)
    }
}