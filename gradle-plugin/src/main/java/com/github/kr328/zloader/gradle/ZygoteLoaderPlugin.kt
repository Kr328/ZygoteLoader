package com.github.kr328.zloader.gradle

import com.android.build.api.variant.ApplicationAndroidComponentsExtension
import com.android.build.gradle.AppExtension
import com.github.kr328.zloader.gradle.decorator.Riru
import org.gradle.api.GradleException
import org.gradle.api.Plugin
import org.gradle.api.Project

class ZygoteLoaderPlugin : Plugin<Project> {
    override fun apply(target: Project) = with(target) {
        if (!target.plugins.hasPlugin("com.android.application")) {
            throw GradleException("Android application plugin not applied")
        }

        val zygote = extensions.create("zygote", ZygoteLoaderExtension::class.java)

        dependencies.apply {
            add("implementation", "com.github.kr328.zloader:runtime:${BuildConfig.VERSION}")
        }
        extensions.configure(AppExtension::class.java) { app ->
            app.flavorDimensions(LOADER_FLAVOR_DIMENSION)
            app.productFlavors { flavors ->
                flavors.create(Riru.NAME) {
                    it.dimension = LOADER_FLAVOR_DIMENSION
                }
                flavors.create("zygisk") {
                    it.dimension = LOADER_FLAVOR_DIMENSION
                }
            }
            app.variantFilter {
                when {
                    it.name.startsWith(Riru.NAME) ->
                        it.ignore = !zygote.riru.isValid
                    it.name.startsWith("zygisk") ->
                        it.ignore = !zygote.zygisk.isValid
                }
            }
        }
        extensions.configure(ApplicationAndroidComponentsExtension::class.java) { components ->
            components.onVariants { app ->
                when (app.flavorName) {
                    Riru.NAME -> {
                        val variantName = app.name
                        val minSdkVersion = app.minSdkVersion
                        val maxSdkVersion = app.maxSdkVersion
                        val versionCode = app.outputs.single().versionCode.orNull ?: 0
                        val versionName = app.outputs.single().versionName.orNull ?: "?"

                        afterEvaluate {
                            Riru.decorateVariant(
                                target,
                                variantName,
                                zygote,
                                versionCode,
                                versionName,
                                minSdkVersion.apiLevel,
                                maxSdkVersion
                            )
                        }
                    }
                    "zygisk" -> {
                        TODO()
                    }
                }
            }
        }
    }

    companion object {
        private const val LOADER_FLAVOR_DIMENSION = "loader"
    }
}