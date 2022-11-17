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
            val agp = "7.3.1"
            val annotation = "1.3.0"
            val riru = "26.0.0"

            library("android-gradle", "com.android.tools.build", "gradle").version(agp)
            library("androidx-annotation", "androidx.annotation", "annotation").version(annotation)
            library("riru-runtime", "dev.rikka.ndk", "riru").version(riru)
        }
    }
}