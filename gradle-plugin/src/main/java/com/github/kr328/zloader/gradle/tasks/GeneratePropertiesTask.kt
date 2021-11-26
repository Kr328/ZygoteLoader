package com.github.kr328.zloader.gradle.tasks

import com.github.kr328.zloader.gradle.util.intermediatesDir
import com.github.kr328.zloader.gradle.util.toCapitalized
import org.gradle.api.DefaultTask
import org.gradle.api.Project
import org.gradle.api.file.DirectoryProperty
import org.gradle.api.provider.MapProperty
import org.gradle.api.tasks.Input
import org.gradle.api.tasks.OutputDirectory
import org.gradle.api.tasks.TaskAction
import org.gradle.api.tasks.TaskProvider
import java.io.File
import java.io.StringWriter
import java.util.*

abstract class GeneratePropertiesTask : DefaultTask() {
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

    companion object Factory {
        fun outputDir(project: Project, variantName: String): File {
            return project.intermediatesDir.resolve("generated_properties").resolve(variantName)
        }

        fun taskName(variantName: String): String {
            return "generateProperties${variantName.toCapitalized()}"
        }

        fun registerOn(
            project: Project,
            variantName: String,
            properties: Map<String, String>,
            configure: GeneratePropertiesTask.() -> Unit = {}
        ): TaskProvider<GeneratePropertiesTask> = with(project) {
            tasks.register(taskName(variantName), GeneratePropertiesTask::class.java) {
                it.properties.putAll(properties)
                it.outputDir.set(outputDir(project, variantName))
                it.configure()
            }
        }
    }
}