plugins {
    java
    `java-gradle-plugin`
    `maven-publish`
}

val dynamicSources = buildDir.resolve("generated/dynamic")

java {
    withSourcesJar()
}

dependencies {
    compileOnly(gradleApi())
    compileOnly(libs.android.gradle)
}

sourceSets {
    named("main") {
        java.srcDir(dynamicSources)
    }
}

gradlePlugin {
    plugins {
        create("zygote") {
            id = "com.github.kr328.gradle.zygote"
            implementationClass = "com.github.kr328.gradle.zygote.ZygoteLoaderPlugin"
        }
    }
}

task("generateDynamicSources") {
    inputs.property("moduleGroup", project.group)
    inputs.property("moduleArtifact", project.name)
    inputs.property("moduleVersion", project.version)
    outputs.dir(dynamicSources)
    tasks.withType(JavaCompile::class.java).forEach { it.dependsOn(this) }
    tasks["sourcesJar"].dependsOn(this)

    doFirst {
        val buildConfig = dynamicSources.resolve("com/github/kr328/gradle/zygote/BuildConfig.java")

        buildConfig.parentFile.mkdirs()

        buildConfig.writeText(
            """
            package com.github.kr328.gradle.zygote;
            
            public final class BuildConfig {
                public static final String RUNTIME_DEPENDENCY = "${project.group}:${project(":runtime").name}:${project.version}";
            }
            """.trimIndent()
        )
    }
}
