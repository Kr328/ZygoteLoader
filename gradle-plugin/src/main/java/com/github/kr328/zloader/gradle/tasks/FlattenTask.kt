package com.github.kr328.zloader.gradle.tasks

import org.gradle.api.DefaultTask
import org.gradle.api.file.DirectoryProperty
import org.gradle.api.tasks.InputDirectory
import org.gradle.api.tasks.OutputDirectory
import org.gradle.api.tasks.TaskAction

abstract class FlattenTask : DefaultTask() {
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
}