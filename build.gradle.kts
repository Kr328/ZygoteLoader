import org.jetbrains.kotlin.gradle.tasks.KotlinCompile

plugins {
    alias(libs.plugins.kotlin.jvm) apply false
}

subprojects {
    group = "com.github.kr328.gradle.zygote"
    version = "2.6"

    plugins.withId("org.jetbrains.kotlin.jvm") {
        tasks.withType(KotlinCompile::class) {
            kotlinOptions {
                jvmTarget = "11"
            }
        }
        extensions.configure<JavaPluginExtension> {
            sourceCompatibility = JavaVersion.VERSION_11
            targetCompatibility = JavaVersion.VERSION_11
        }
    }

    plugins.withId("maven-publish") {
        extensions.configure<PublishingExtension> {
            repositories {
                mavenLocal()

                maven {
                    name = "kr328app"
                    url = uri("https://maven.kr328.app/releases")
                    credentials(PasswordCredentials::class.java)
                }
            }
            publications {
                create(project.name, MavenPublication::class) {
                    pom {
                        name.set("ZygoteLoader")
                        description.set("ZygoteLoader")
                        url.set("https://github.com/Kr328/ZygoteLoader")
                        licenses {
                            license {
                                name.set("MIT License")
                            }
                        }
                        developers {
                            developer {
                                name.set("Kr328")
                            }
                        }
                        scm {
                            connection.set("scm:git:https://github.com/Kr328/ZygoteLoader.git")
                            url.set("https://github.com/Kr328/ZygoteLoader.git")
                        }
                    }

                    groupId = project.group.toString()
                    version = project.version.toString()
                    artifactId = project.name
                }
            }
        }
    }
}

task("clean", type = Delete::class) {
    delete(rootProject.buildDir)
}
