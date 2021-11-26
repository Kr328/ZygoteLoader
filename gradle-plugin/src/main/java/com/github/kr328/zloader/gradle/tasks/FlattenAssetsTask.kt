package com.github.kr328.zloader.gradle.tasks

import com.github.kr328.zloader.gradle.util.intermediatesDir
import com.github.kr328.zloader.gradle.util.toCapitalized
import org.gradle.api.DefaultTask
import org.gradle.api.Project
import org.gradle.api.file.DirectoryProperty
import org.gradle.api.tasks.InputDirectory
import org.gradle.api.tasks.OutputDirectory
import org.gradle.api.tasks.TaskAction
import org.gradle.api.tasks.TaskProvider
import java.io.File

abstract class FlattenAssetsTask : DefaultTask() {
    @get:InputDirectory
    abstract val assetsDir: DirectoryProperty

    @get:OutputDirectory
    abstract val outputDir: DirectoryProperty

    @TaskAction
    fun doAction() {
        project.sync { sync ->
            sync.includeEmptyDirs = false

            sync.into(outputDir)

            sync.from(assetsDir) {
                it.exclude("**/*.jar")
            }

            assetsDir.asFile.get().walk()
                .filter { it.extension == "jar" }
                .forEach { jar ->
                    sync.from(project.zipTree(jar))
                }
        }
    }

    companion object Factory {
        fun outputDir(project: Project, variantName: String): File {
            return project.intermediatesDir.resolve("flatten_assets").resolve(variantName)
        }

        fun taskName(variantName: String): String {
            return "flattenAssets${variantName.toCapitalized()}"
        }

        fun registerOn(
            project: Project,
            variantName: String,
            assetsDir: DirectoryProperty,
            configure: FlattenAssetsTask.() -> Unit = {},
        ): TaskProvider<FlattenAssetsTask> = with(project) {
            tasks.register(taskName(variantName), FlattenAssetsTask::class.java) {
                it.assetsDir.set(assetsDir)
                it.outputDir.set(outputDir(this, variantName))
                it.configure()
            }
        }
    }
}