import org.jetbrains.kotlin.gradle.tasks.KotlinCompile

plugins {
    kotlin("jvm") version "1.6.10"
    `java-gradle-plugin`
}

val dynamicSources = buildDir.resolve("generated/dynamic")

java {
    sourceCompatibility = JavaVersion.VERSION_1_8
    targetCompatibility = JavaVersion.VERSION_1_8
}

dependencies {
    compileOnly(gradleApi())
    compileOnly("com.android.tools.build:gradle:7.1.0")
}

sourceSets {
    named("main") {
        java.srcDir(dynamicSources)
    }
}

gradlePlugin {
    plugins {
        create("zygote-loader") {
            id = "zygote-loader"
            implementationClass = "com.github.kr328.zloader.gradle.ZygoteLoaderPlugin"
        }
    }
}

task("sourcesJar", Jar::class) {
    archiveClassifier.set("sources")

    from(sourceSets["main"].allSource)
}

task("generateDynamicSources") {
    inputs.property("version", version)
    outputs.dir(dynamicSources)
    tasks.withType(JavaCompile::class.java).forEach { it.dependsOn(this) }
    tasks.withType(KotlinCompile::class.java).forEach { it.dependsOn(this) }

    doFirst {
        val buildConfig = dynamicSources.resolve("com/github/kr328/zloader/gradle/BuildConfig.java")

        buildConfig.parentFile.mkdirs()

        buildConfig.writeText(
            """
            package com.github.kr328.zloader.gradle;
            
            public final class BuildConfig {
                public static final String VERSION = "$version";
            }
            """.trimIndent()
        )
    }
}

afterEvaluate {
    tasks["sourcesJar"].dependsOn(tasks["generateDynamicSources"])

    publishing {
        publications {
            create("gradle-plugin", MavenPublication::class) {
                from(components["java"])

                artifact(tasks["sourcesJar"])
            }
        }
    }
}
