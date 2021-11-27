import com.android.build.gradle.BaseExtension

// Top-level build file where you can add configuration options common to all sub-projects/modules.
buildscript {
    repositories {
        google()
        mavenCentral()
    }
    dependencies {
        classpath("com.android.tools.build:gradle:7.0.3")
    }
}

allprojects {
    group = "com.github.kr328.zloader"
    version = "1.0"

    repositories {
        google()
        mavenCentral()
    }
}

subprojects {
    apply(plugin = "maven-publish")

    afterEvaluate {
        group = "com.github.kr328.zloader"
        version = "1.3"

        val isAndroid = plugins.hasPlugin("com.android.base")

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
                create("main", MavenPublication::class.java) {
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

                    afterEvaluate {
                        from(if (isAndroid) components["release"] else components["java"])
                    }

                    val sourcesJar = tasks.register("sourcesJar", type = Jar::class) {
                        archiveClassifier.set("sources")

                        val extensions = project.extensions
                        if (isAndroid) {
                            from(extensions.getByType(BaseExtension::class.java).sourceSets["main"].java.srcDirs)
                        } else {
                            from(extensions.getByType(SourceSetContainer::class.java)["main"].allSource)
                        }
                    }

                    artifact(sourcesJar)

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
