package com.github.kr328.zloader.gradle.tasks

import org.gradle.api.DefaultTask
import org.gradle.api.file.DirectoryProperty
import org.gradle.api.provider.MapProperty
import org.gradle.api.tasks.Input
import org.gradle.api.tasks.OutputDirectory
import org.gradle.api.tasks.TaskAction
import java.io.StringWriter
import java.util.*

abstract class PropertiesTask : DefaultTask() {
    @get:Input
    abstract val properties: MapProperty<String, String>

    @get:OutputDirectory
    abstract val outputDir: DirectoryProperty

    @TaskAction
    fun doAction() {
        val writer = StringWriter()

        Properties().apply {
            putAll(properties.get())
            store(writer, null)
        }

        outputDir.get().asFile.apply {
            mkdirs()

            val text = writer.toString().lineSequence()
                .filterNot { it.startsWith("#") }
                .joinToString(separator = "\n")

            resolve("module.prop").writeText(text)
        }
    }
}