package com.github.kr328.zloader.gradle.tasks

import org.gradle.api.DefaultTask
import org.gradle.api.file.DirectoryProperty
import org.gradle.api.provider.SetProperty
import org.gradle.api.tasks.Input
import org.gradle.api.tasks.OutputDirectory
import org.gradle.api.tasks.TaskAction

abstract class PackagesTask : DefaultTask() {
    @get:Input
    abstract val packages: SetProperty<String>

    @get:OutputDirectory
    abstract val destinationDir: DirectoryProperty

    @TaskAction
    fun doAction() {
        destinationDir.asFile.get().resolve("packages").apply {
            deleteRecursively()

            mkdirs()

            packages.get().forEach {
                resolve(it).createNewFile()
            }
        }
    }
}