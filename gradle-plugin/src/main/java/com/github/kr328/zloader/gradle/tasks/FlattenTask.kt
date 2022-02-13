package com.github.kr328.zloader.gradle.tasks

import com.github.kr328.zloader.gradle.util.fromKtx
import com.github.kr328.zloader.gradle.util.syncKtx
import org.gradle.api.DefaultTask
import org.gradle.api.file.DirectoryProperty
import org.gradle.api.tasks.InputDirectory
import org.gradle.api.tasks.OutputDirectory
import org.gradle.api.tasks.TaskAction

abstract class FlattenTask : DefaultTask() {
    @get:InputDirectory
    abstract val composedDir: DirectoryProperty

    @get:OutputDirectory
    abstract val destinationDir: DirectoryProperty

    @TaskAction
    fun doAction() {
        project.syncKtx {
            includeEmptyDirs = false

            into(destinationDir)

            fromKtx(composedDir) {
                exclude("**/*.jar")
            }

            composedDir.asFile.get().walk()
                .filter { it.extension == "jar" }
                .forEach { jar ->
                    from(project.zipTree(jar))
                }
        }
    }
}
