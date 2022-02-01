package com.github.kr328.zloader.gradle.tasks

import org.gradle.api.DefaultTask
import org.gradle.api.file.DirectoryProperty
import org.gradle.api.provider.MapProperty
import org.gradle.api.tasks.Input
import org.gradle.api.tasks.OutputDirectory
import org.gradle.api.tasks.TaskAction

abstract class PropertiesTask : DefaultTask() {
    @get:Input
    abstract val properties: MapProperty<String, String>

    @get:OutputDirectory
    abstract val outputDir: DirectoryProperty

    @TaskAction
    fun doAction() {
        val text = properties.get().toList().joinToString(separator = "\n", postfix = "\n") {
            "${it.first}=${it.second}"
        }

        outputDir.get().asFile.apply {
            mkdirs()

            resolve("module.prop").writeText(text)
        }
    }
}