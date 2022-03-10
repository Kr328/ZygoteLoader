package com.github.kr328.gradle.zygote

import com.android.build.api.variant.ApplicationVariant
import com.android.build.gradle.internal.scope.InternalArtifactType
import com.android.build.gradle.internal.scope.InternalMultipleArtifactType
import com.github.kr328.gradle.zygote.compat.resolveImpl
import com.github.kr328.gradle.zygote.tasks.*
import com.github.kr328.gradle.zygote.util.*
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
        val artifacts = variant.artifacts.resolveImpl()
        val capitalized = variant.name.toCapitalized()
        val packing = tasks.named("package$capitalized")

        val moduleProp = sequence {
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

        val generateModuleProp = tasks.registerKtx(
            "generateModuleProp$capitalized",
            PropertiesTask::class,
        ) {
            properties.putAll(moduleProp)

            fileName.set("module.prop")

            destinationDir.set(buildDir.resolve("generated/properties/${variant.name}"))
        }

        val generateInitialPackages = tasks.registerKtx(
            "generateInitialPackages$capitalized",
            PackagesTask::class,
        ) {
            packages.addAll(extension.packages)

            destinationDir.set(buildDir.resolve("generated/packages/${variant.name}"))
        }

        val flattenAssets = tasks.registerKtx(
            "flattenAssets$capitalized",
            FlattenTask::class
        ) {
            dependsOn(packing)

            composedDir.set(artifacts.get(InternalArtifactType.COMPRESSED_ASSETS))

            destinationDir.set(buildDir.resolve("intermediates/flatten_assets/${variant.name}"))
        }

        val mergeMagisk = tasks.registerKtx(
            "mergeMagisk$capitalized",
            Sync::class
        ) {
            val nativeLibs = artifacts.get(InternalArtifactType.MERGED_NATIVE_LIBS)
            val dex = artifacts.getAll(InternalMultipleArtifactType.DEX).map { it.single() }

            dependsOn(flattenAssets, generateInitialPackages, generateModuleProp, packing)

            destinationDir = buildDir.resolve("intermediates/merged_magisk/${variant.name}")

            fromKtx(nativeLibs.asFile().resolve("lib")) {
                when (loader) {
                    Loader.Riru -> {
                        include("**/libriru_loader.so")
                        into("riru")
                        rename {
                            if (it == "libriru_loader.so") "lib${moduleProp["id"]}.so" else it
                        }
                    }
                    Loader.Zygisk -> {
                        include("**/libzygisk_loader.so")
                        into("zygisk")
                        eachFile {
                            it.path = it.path.replace("/libzygisk_loader", "")
                        }
                    }
                }
            }
            fromKtx(dex) {
                include("classes.dex")
            }
            fromKtx(flattenAssets.get().destinationDir.asFile.resolve("assets"))
            fromKtx(generateInitialPackages.get().destinationDir)
            fromKtx(generateModuleProp.get().destinationDir)
        }

        val generateChecksum = tasks.registerKtx(
            "generateChecksum$capitalized",
            ChecksumTask::class
        ) {
            dependsOn(mergeMagisk)

            inputDir.set(mergeMagisk.get().destinationDir)
            destinationDir.set(buildDir.resolve("generated/checksum/${variant.name}"))
        }

        val generateCustomize = tasks.registerKtx(
            "generateCustomize$capitalized",
            CustomizeTask::class,
        ) {
            dependsOn(mergeMagisk, generateChecksum)

            destinationDir.set(buildDir.resolve("generated/customize_sh/${variant.name}"))

            customizeFiles.set(mergeMagisk.get().destinationDir.resolve("customize.d"))
            checksumFiles.set(generateChecksum.get().destinationDir.get().asFile.resolve("customize.d"))
        }

        val packagingMagisk = tasks.registerKtx(
            "packageMagisk$capitalized",
            Zip::class
        ) {
            dependsOn(mergeMagisk, generateCustomize)

            val destinationDir = buildDir.resolve("outputs/magisk")
                .resolve("${variant.flavorName}")
                .resolve("${variant.buildType}")
            val archiveName = when (loader) {
                Loader.Riru -> extension.riru["archiveName"]
                Loader.Zygisk -> extension.zygisk["archiveName"]
            } ?: project.name

            destinationDirectory.set(destinationDir)
            archiveBaseName.set(archiveName)
            includeEmptyDirs = false
            entryCompression = ZipEntryCompression.DEFLATED
            isPreserveFileTimestamps = false

            fromKtx(mergeMagisk.get().destinationDir)
            fromKtx(generateCustomize.get().destinationDir)
            fromKtx(generateChecksum.get().destinationDir)
        }

        tasks.getByName("assemble$capitalized").dependsOn(packagingMagisk)
    }
}