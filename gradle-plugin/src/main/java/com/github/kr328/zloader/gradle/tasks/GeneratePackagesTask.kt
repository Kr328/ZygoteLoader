package com.github.kr328.zloader.gradle.tasks

import com.github.kr328.zloader.gradle.util.intermediatesDir
import com.github.kr328.zloader.gradle.util.toCapitalized
import org.gradle.api.DefaultTask
import org.gradle.api.Project
import org.gradle.api.file.DirectoryProperty
import org.gradle.api.provider.SetProperty
import org.gradle.api.tasks.Input
import org.gradle.api.tasks.OutputDirectory
import org.gradle.api.tasks.TaskAction
import org.gradle.api.tasks.TaskProvider
import java.io.File

abstract class GeneratePackagesTask : DefaultTask() {
    @get:Input
    abstract val packages: SetProperty<String>

    @get:OutputDirectory
    abstract val outputDir: DirectoryProperty

    @TaskAction
    fun doAction() {
        val file = outputDir.asFile.get().resolve("packages")

        file.deleteRecursively()
        file.mkdirs()

        packages.get().forEach {
            file.resolve(it).createNewFile()
        }
    }

    companion object Factory {
        fun outputDir(project: Project, variantName: String): File {
            return project.intermediatesDir.resolve("generated_packages").resolve(variantName)
        }

        fun taskName(variantName: String): String {
            return "generatePackages${variantName.toCapitalized()}"
        }

        fun registerOn(
            project: Project,
            variantName: String,
            packages: Set<String>,
            configure: GeneratePackagesTask.() -> Unit = {},
        ): TaskProvider<GeneratePackagesTask> = with(project) {
            tasks.register(taskName(variantName), GeneratePackagesTask::class.java) {
                it.packages.addAll(packages)
                it.outputDir.set(outputDir(project, variantName))
                it.configure()
            }
        }
    }
}