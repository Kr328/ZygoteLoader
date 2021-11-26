package com.github.kr328.zloader.gradle.tasks

import com.github.kr328.zloader.gradle.util.toCapitalized
import org.gradle.api.Project
import org.gradle.api.tasks.TaskProvider
import org.gradle.api.tasks.bundling.Zip
import org.gradle.api.tasks.bundling.ZipEntryCompression
import java.io.File

abstract class PackageMagiskTask : Zip() {
    companion object Factory {
        fun outputDir(project: Project, variantName: String): File {
            return project.buildDir.resolve("outputs/magisk").resolve(variantName)
        }

        fun taskName(variantName: String): String {
            return "packageMagisk${variantName.toCapitalized()}"
        }

        fun registerOn(
            project: Project,
            variantName: String,
            configure: PackageMagiskTask.() -> Unit
        ): TaskProvider<PackageMagiskTask> = with(project) {
            tasks.register(taskName(variantName), PackageMagiskTask::class.java) {
                it.destinationDirectory.set(outputDir(this, variantName))
                it.archiveBaseName.set(project.name)
                it.includeEmptyDirs = false
                it.entryCompression = ZipEntryCompression.DEFLATED
                it.isPreserveFileTimestamps = false

                it.rename { name ->
                    if (name == "dist-gitattributes") ".gitattributes" else name
                }

                it.configure()
            }
        }
    }
}