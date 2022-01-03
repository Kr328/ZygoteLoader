package com.github.kr328.zloader.gradle.tasks

import org.gradle.api.DefaultTask
import org.gradle.api.file.DirectoryProperty
import org.gradle.api.file.RegularFileProperty
import org.gradle.api.tasks.InputFile
import org.gradle.api.tasks.OutputFile
import org.gradle.api.tasks.TaskAction

abstract class FlattenZipTask : DefaultTask() {
    @get:InputFile
    abstract val file: RegularFileProperty

    @get:OutputFile
    abstract val output: DirectoryProperty

    @TaskAction
    fun doAction() {
        project.sync {
            it.from(project.zipTree(file))
            it.into(output)
        }
    }
}