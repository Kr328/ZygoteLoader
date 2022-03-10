package com.github.kr328.gradle.zygote.tasks

import org.gradle.api.DefaultTask
import org.gradle.api.file.DirectoryProperty
import org.gradle.api.provider.MapProperty
import org.gradle.api.provider.Property
import org.gradle.api.tasks.Input
import org.gradle.api.tasks.OutputDirectory
import org.gradle.api.tasks.TaskAction

abstract class PropertiesTask : DefaultTask() {
    @get:Input
    abstract val properties: MapProperty<String, String>

    @get:Input
    abstract val fileName: Property<String>

    @get:OutputDirectory
    abstract val destinationDir: DirectoryProperty

    @TaskAction
    fun doAction() {
        val text = properties.get().toList().joinToString(separator = "\n", postfix = "\n") {
            "${it.first}=${it.second}"
        }

        destinationDir.get().asFile.apply {
            mkdirs()

            resolve(fileName.get()).writeText(text)
        }
    }
}
