package com.github.kr328.zloader.gradle

import com.android.build.api.variant.ApplicationVariant
import com.android.build.gradle.internal.scope.InternalArtifactType
import com.android.build.gradle.internal.scope.InternalMultipleArtifactType
import com.github.kr328.zloader.gradle.tasks.*
import com.github.kr328.zloader.gradle.util.resolveImpl
import com.github.kr328.zloader.gradle.util.toCapitalized
import org.gradle.api.Project
import org.gradle.api.tasks.Sync
import org.gradle.api.tasks.bundling.Zip
import org.gradle.api.tasks.bundling.ZipEntryCompression

object ZygoteLoaderDecorator {
    enum class Loader(val flavorName: String) {
        Riru("riru"), Zygisk("zygisk")
    }

    fun decorateVariant(
        loader: Loader,
        project: Project,
        variant: ApplicationVariant,
        extension: ZygoteLoaderExtension,
    ): Unit = with(project) {
        afterEvaluate {
            val artifacts = variant.artifacts.resolveImpl()
            val capitalized = variant.name.toCapitalized()
            val packing = tasks.getByName("package$capitalized")

            val properties = sequence {
                val output = variant.outputs.single()

                yield("version" to (output.versionName.orNull ?: ""))
                yield("versionCode" to (output.versionCode.orNull ?: 0).toString())
                yield("minSdkVersion" to (variant.minSdkVersion.apiLevel.toString()))
                if (variant.maxSdkVersion != null) {
                    yield("maxSdkVersion" to variant.maxSdkVersion!!.toString())
                }
            }.toMap() + when (loader) {
                Loader.Riru -> extension.riru
                Loader.Zygisk -> extension.zygisk
            }

            val generateProperties = tasks.register(
                "generateProperties$capitalized",
                PropertiesTask::class.java,
            ) {
                it.outputDir.set(buildDir.resolve("generated/properties/${variant.name}"))
                it.properties.putAll(properties)
            }

            val generatePackages = tasks.register(
                "generatePackages$capitalized",
                PackagesTask::class.java,
            ) {
                it.outputDir.set(buildDir.resolve("generated/packages/${variant.name}"))
                it.packages.addAll(extension.packages)
            }

            val flattenAssets = tasks.register(
                "flattenAssets$capitalized",
                FlattenTask::class.java
            ) {
                it.dependsOn(packing)
                it.outputDir.set(buildDir.resolve("intermediates/flatten_assets/${variant.name}"))
                it.assetsDir.set(artifacts.get(InternalArtifactType.COMPRESSED_ASSETS))
            }

            val mergeMagisk = tasks.register(
                "mergeMagisk$capitalized",
                Sync::class.java
            ) { sync ->
                val nativeLibs = artifacts.get(InternalArtifactType.MERGED_NATIVE_LIBS)
                val dex = artifacts.getAll(InternalMultipleArtifactType.DEX).map { it.single() }

                sync.dependsOn(flattenAssets, generatePackages, generateProperties, packing)

                sync.destinationDir = buildDir.resolve("intermediates/merged_magisk/${variant.name}")

                sync.from(nativeLibs.map { it.asFile.resolve("lib") }) { spec ->
                    when (loader) {
                        Loader.Riru -> {
                            spec.include("**/libriru_loader.so")
                            spec.into("riru")
                            spec.rename {
                                if (it == "libriru_loader.so") "lib${properties["id"]}.so" else it
                            }
                        }
                        Loader.Zygisk -> {
                            spec.include("**/libzygisk_loader.so")
                            spec.into("zygisk")
                            spec.eachFile {
                                it.path = it.path.replace("/libzygisk_loader", "")
                            }
                        }
                    }
                }
                sync.from(dex) {
                    it.include("classes.dex")
                }
                sync.from(flattenAssets.get().outputDir.get().asFile.resolve("assets"))
                sync.from(generatePackages.get().outputDir)
                sync.from(generateProperties.get().outputDir)
            }

            val generateChecksum = tasks.register(
                "generateChecksum$capitalized",
                ChecksumTask::class.java
            ) {
                it.dependsOn(mergeMagisk)

                it.inputDir.set(mergeMagisk.get().destinationDir)
                it.outputDir.set(buildDir.resolve("generated/checksum/${variant.name}"))
            }

            val generateCustomize = tasks.register(
                "generateCustomize$capitalized",
                CustomizeTask::class.java,
            ) {
                it.dependsOn(mergeMagisk, generateChecksum)

                it.outputDir.set(buildDir.resolve("generated/customize_sh/${variant.name}"))

                it.customizeFiles.set(mergeMagisk.get().destinationDir.resolve("customize.d"))
                it.checksumFiles.set(generateChecksum.get().outputDir.get().asFile.resolve("customize.d"))
            }

            val packagingMagisk = tasks.register(
                "packageMagisk$capitalized",
                Zip::class.java
            ) { zip ->
                zip.dependsOn(mergeMagisk, generateCustomize)

                val outputDir = buildDir.resolve("outputs/magisk")
                    .resolve("${variant.flavorName}")
                    .resolve("${variant.buildType}")
                val archiveName = when (loader) {
                    Loader.Riru -> extension.riru["archiveName"]
                    Loader.Zygisk -> extension.zygisk["archiveName"]
                } ?: project.name

                zip.destinationDirectory.set(outputDir)
                zip.archiveBaseName.set(archiveName)
                zip.includeEmptyDirs = false
                zip.entryCompression = ZipEntryCompression.DEFLATED
                zip.isPreserveFileTimestamps = false

                zip.from(mergeMagisk.get().destinationDir)
                zip.from(generateCustomize.get().outputDir)
                zip.from(generateChecksum.get().outputDir)
            }

            tasks.getByName("assemble$capitalized").dependsOn(packagingMagisk)
        }
    }
}