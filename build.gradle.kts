// Top-level build file where you can add configuration options common to all sub-projects/modules.
buildscript {
    repositories {
        google()
        mavenCentral()
    }
    dependencies {
        classpath("com.android.tools.build:gradle:7.0.4")
    }
}

allprojects {
    repositories {
        google()
        mavenCentral()
    }
}

subprojects {
    group = "com.github.kr328.zloader"
    version = "1.7"

    apply(plugin = "maven-publish")

    extensions.configure(PublishingExtension::class.java) {
        repositories {
            mavenLocal()

            maven {
                name = "kr328app"
                url = uri("https://maven.kr328.app/releases")
                credentials(PasswordCredentials::class.java)
            }
        }
        publications {
            afterEvaluate {
                afterEvaluate {
                    withType(MavenPublication::class) {
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
                        artifactId = name
                    }
                }
            }
        }
    }
}

task("clean", type = Delete::class) {
    delete(rootProject.buildDir)
}
