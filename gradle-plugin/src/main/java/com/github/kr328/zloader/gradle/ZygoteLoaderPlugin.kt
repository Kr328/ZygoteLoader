package com.github.kr328.zloader.gradle

import com.android.build.api.variant.ApplicationAndroidComponentsExtension
import com.github.kr328.zloader.gradle.util.*
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

        dependencies.add(
            "implementation",
            "com.github.kr328.zloader:runtime:${BuildConfig.VERSION}"
        )

        extensions.configureKtx(ApplicationAndroidComponentsExtension::class) {
            finalizeDslKtx {
                flavorDimensions += LOADER_FLAVOR_DIMENSION
                productFlavors {
                    createKtx(ZygoteLoaderDecorator.Loader.Riru.flavorName) {
                        dimension = LOADER_FLAVOR_DIMENSION
                        multiDexEnabled = false
                    }
                    createKtx(ZygoteLoaderDecorator.Loader.Zygisk.flavorName) {
                        dimension = LOADER_FLAVOR_DIMENSION
                        multiDexEnabled = false
                    }
                }
            }
            onVariantsKtx {
                val loader = when (flavorName) {
                    ZygoteLoaderDecorator.Loader.Riru.flavorName ->
                        ZygoteLoaderDecorator.Loader.Riru
                    ZygoteLoaderDecorator.Loader.Zygisk.flavorName ->
                        ZygoteLoaderDecorator.Loader.Zygisk
                    else -> null
                }

                afterEvaluate {
                    if (loader != null) {
                        ZygoteLoaderDecorator.decorateVariant(
                            loader,
                            target,
                            this,
                            zygote
                        )
                    }
                }
            }
        }
    }

    companion object {
        private const val LOADER_FLAVOR_DIMENSION = "loader"
    }
}