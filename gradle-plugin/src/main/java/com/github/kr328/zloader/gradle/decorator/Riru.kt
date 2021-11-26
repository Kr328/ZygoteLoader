package com.github.kr328.zloader.gradle.decorator

import com.android.build.gradle.tasks.PackageAndroidArtifact
import com.github.kr328.zloader.gradle.ZygoteLoaderExtension
import com.github.kr328.zloader.gradle.tasks.FlattenAssetsTask
import com.github.kr328.zloader.gradle.tasks.GeneratePackagesTask
import com.github.kr328.zloader.gradle.tasks.GeneratePropertiesTask
import com.github.kr328.zloader.gradle.tasks.PackageMagiskTask
import com.github.kr328.zloader.gradle.util.toCapitalized
import org.gradle.api.Project

object Riru {
    const val NAME = "riru"

    fun decorateVariant(
        target: Project,
        variantName: String,
        extension: ZygoteLoaderExtension,
        versionCode: Int,
        versionName: String,
        minSdkVersion: Int,
        maxSdkVersion: Int?,
    ): Unit = with(target) {
        val capitalizedName = variantName.toCapitalized()
        val packaging = tasks.getByName("package$capitalizedName") as PackageAndroidArtifact
        val assemble = tasks.getByName("assemble$capitalizedName")

        val flattenAssets = FlattenAssetsTask.registerOn(
            this,
            variantName,
            packaging.assets
        ) {
            dependsOn(packaging)
        }

        val properties = sequence {
            yield("version" to versionName)
            yield("versionCode" to versionCode.toString())
            yield("minSdkVersion" to minSdkVersion.toString())
            if (maxSdkVersion != null) yield("maxSdkVersion" to maxSdkVersion.toString())
        }.toMap() + extension.riru
        val generateProperties = GeneratePropertiesTask.registerOn(
            this,
            variantName,
            properties
        )

        val generatePackages = GeneratePackagesTask.registerOn(
            this,
            variantName,
            extension.packages
        )

        val packageMagisk = PackageMagiskTask.registerOn(this, variantName) {
            dependsOn(flattenAssets, generatePackages, generatePackages)

            from(packaging.jniFolders.map { it.resolve("lib") }) {
                it.into("riru")
                it.include("**/*.so")
            }
            from(packaging.dexFolders) {
                it.include("classes.dex")
            }
            from(flattenAssets.get().outputDir.file("assets/riru"))
            from(flattenAssets.get().outputDir.file("assets")) {
                it.exclude("riru/**", "zygisk/**")
            }
            from(generatePackages.get().outputDir)
            from(generateProperties.get().outputDir)
        }

        assemble.dependsOn(packageMagisk)
    }
}