@file:Suppress("UnstableApiUsage")

rootProject.name = "ZygoteLoader"

include(":runtime")
include(":gradle-plugin")

pluginManagement {
    repositories {
        mavenCentral()
        google()
        gradlePluginPortal()
    }
}

dependencyResolutionManagement {
    repositories {
        mavenCentral()
        google()
    }
    versionCatalogs {
        create("libs") {
            val kotlin = "1.6.10"
            val agp = "7.1.2"
            val annotation = "1.3.0"
            val riru = "26.0.0"

            plugin("kotlin-jvm", "org.jetbrains.kotlin.jvm").version(kotlin)
            plugin("android-library", "com.android.library").version(agp)
            library("android-gradle", "com.android.tools.build", "gradle").version(agp)
            library("androidx-annotation", "androidx.annotation", "annotation").version(annotation)
            library("riru-runtime", "dev.rikka.ndk", "riru").version(riru)
        }
    }
}