import org.jetbrains.kotlin.gradle.tasks.KotlinCompile

plugins {
    alias(deps.plugins.kotlin.jvm)
    `java-gradle-plugin`
    java
}

val dynamicSources = buildDir.resolve("generated/dynamic")

java {
    sourceCompatibility = JavaVersion.VERSION_1_8
    targetCompatibility = JavaVersion.VERSION_1_8
}

dependencies {
    compileOnly(gradleApi())
    compileOnly(deps.android.gradle)
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
    inputs.property("version", version)
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
                public static final String VERSION = "$version";
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
