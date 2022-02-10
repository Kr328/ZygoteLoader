package com.github.kr328.zloader.gradle

import com.android.build.api.variant.ApplicationAndroidComponentsExtension
import org.gradle.api.GradleException
import org.gradle.api.Plugin
import org.gradle.api.Project

@Suppress("UnstableApiUsage")
class ZygoteLoaderPlugin : Plugin<Project> {
    override fun apply(target: Project) = with(target) {
        if (!target.plugins.hasPlugin("com.android.application")) {
            throw GradleException("Android application plugin not applied")
        }

        val zygote = extensions.create("zygote", ZygoteLoaderExtension::class.java)

        extensions.configure(ApplicationAndroidComponentsExtension::class.java) { components ->
            components.finalizeDsl { app ->
                app.flavorDimensions += LOADER_FLAVOR_DIMENSION
                app.productFlavors {
                    create(ZygoteLoaderDecorator.Loader.Riru.flavorName) {
                        it.dimension = LOADER_FLAVOR_DIMENSION
                        it.multiDexEnabled = false
                    }
                    create(ZygoteLoaderDecorator.Loader.Zygisk.flavorName) {
                        it.dimension = LOADER_FLAVOR_DIMENSION
                        it.multiDexEnabled = false
                    }
                }
            }
            components.beforeVariants { app ->
                afterEvaluate {
                    when (app.flavorName) {
                        ZygoteLoaderDecorator.Loader.Riru.flavorName -> {
                            dependencies.add(
                                "${app.name}Implementation",
                                "com.github.kr328.zloader:runtime-riru${if (app.debuggable) "-debug" else ""}:${BuildConfig.VERSION}"
                            )
                        }
                        ZygoteLoaderDecorator.Loader.Zygisk.flavorName -> {
                            dependencies.add(
                                "${app.name}Implementation",
                                "com.github.kr328.zloader:runtime-zygisk${if (app.debuggable) "-debug" else ""}:${BuildConfig.VERSION}"
                            )
                        }
                    }
                }

                app.enable = when (app.flavorName) {
                    ZygoteLoaderDecorator.Loader.Riru.flavorName -> zygote.riru.isValid
                    ZygoteLoaderDecorator.Loader.Zygisk.flavorName -> zygote.zygisk.isValid
                    else -> app.enable
                }
            }
            components.onVariants { app ->
                val loader = when (app.flavorName) {
                    ZygoteLoaderDecorator.Loader.Riru.flavorName ->
                        ZygoteLoaderDecorator.Loader.Riru
                    ZygoteLoaderDecorator.Loader.Zygisk.flavorName ->
                        ZygoteLoaderDecorator.Loader.Zygisk
                    else -> null
                }

                if (loader != null) {
                    ZygoteLoaderDecorator.decorateVariant(
                        loader,
                        target,
                        app,
                        zygote
                    )
                }
            }
        }
    }

    companion object {
        private const val LOADER_FLAVOR_DIMENSION = "loader"
    }
}