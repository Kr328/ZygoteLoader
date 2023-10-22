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
            val agp = "8.1.2"
            val annotation = "1.7.0"
            val riru = "26.0.0"

            library("android-gradle", "com.android.tools.build:gradle:$agp")
            library("androidx-annotation", "androidx.annotation:annotation:$annotation")
            library("riru-runtime", "dev.rikka.ndk:riru:$riru")
        }
    }
}