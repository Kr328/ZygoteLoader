plugins {
    id("com.android.library") version "8.1.2" apply false
}

subprojects {
    group = "com.github.kr328.gradle.zygote"
    version = "3.0"

    plugins.withId("java") {
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
                withType<MavenPublication> {
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
                }
            }
        }
    }
}

task("clean", type = Delete::class) {
    delete(rootProject.buildDir)
}
