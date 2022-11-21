package com.github.kr328.gradle.zygote.tasks

import com.github.kr328.gradle.zygote.util.fromKtx
import com.github.kr328.gradle.zygote.util.syncKtx
import org.apache.tools.ant.filters.FixCrLfFilter
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

            val crlf: Map<String, *> = mapOf("eol" to FixCrLfFilter.CrLf.newInstance("lf"))
            eachFile {
                if (it.relativePath.startsWith("assets/META-INF") || it.name.endsWith(".sh")) {
                    it.filter(crlf, FixCrLfFilter::class.java)
                }
            }
        }
    }
}
