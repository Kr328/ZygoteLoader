import org.jetbrains.kotlin.gradle.tasks.KotlinCompile

plugins {
    alias(libs.plugins.kotlin.jvm)
    `java-gradle-plugin`
    `maven-publish`
}

val dynamicSources = buildDir.resolve("generated/dynamic")

java {
    sourceCompatibility = JavaVersion.VERSION_1_8
    targetCompatibility = JavaVersion.VERSION_1_8
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

task("sourcesJar", Jar::class) {
    archiveClassifier.set("sources")

    from(sourceSets["main"].allSource)
}

task("generateDynamicSources") {
    inputs.property("moduleGroup", project.group)
    inputs.property("moduleArtifact", project.name)
    inputs.property("moduleVersion", project.version)
    outputs.dir(dynamicSources)
    tasks.withType(JavaCompile::class.java).forEach { it.dependsOn(this) }
    tasks.withType(KotlinCompile::class.java).forEach { it.dependsOn(this) }

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

publishing {
    publications {
        named(project.name, MavenPublication::class) {
            from(components["java"])

            artifact(tasks["sourcesJar"])
        }
    }
}

afterEvaluate {
    tasks["sourcesJar"].dependsOn(tasks["generateDynamicSources"])
}
