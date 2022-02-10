plugins {
    id("com.android.library")
    `maven-publish`
}

android {
    compileSdk = 31
    ndkVersion = "23.1.7779620"

    defaultConfig {
        minSdk = 26
        targetSdk = 31

        consumerProguardFiles("consumer-rules.pro")

        externalNativeBuild {
            cmake {
                abiFilters("arm64-v8a", "armeabi-v7a", "x86", "x86_64")
                arguments("-DANDROID_STL=none")
            }
        }
    }

    productFlavors {
        flavorDimensions.add("loader")
        create("riru") {
            dimension = flavorDimensions[0]

            externalNativeBuild {
                cmake {
                    arguments("-DLOADER:STRING=riru")
                }
            }
        }
        create("zygisk") {
            dimension = flavorDimensions[0]

            externalNativeBuild {
                cmake {
                    arguments("-DLOADER:STRING=zygisk")
                }
            }
        }
    }
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.18.1"
        }
    }
    buildFeatures {
        prefab = true
    }
}

dependencies {
    implementation("dev.rikka.ndk:riru:26.0.0")
    implementation("dev.rikka.ndk.thirdparty:cxx:1.2.0")
}

task("sourcesJar", Jar::class) {
    archiveClassifier.set("sources")

    from(android.sourceSets["main"].java.srcDirs)
}

afterEvaluate {
    publishing {
        publications {
            create("runtime-riru", MavenPublication::class) {
                from(components["riruRelease"])
            }
            create("runtime-riru-debug", MavenPublication::class) {
                from(components["riruDebug"])
            }
            create("runtime-zygisk", MavenPublication::class) {
                from(components["zygiskRelease"])
            }
            create("runtime-zygisk-debug", MavenPublication::class) {
                from(components["zygiskDebug"])
            }
            withType(MavenPublication::class) {
                artifact(tasks["sourcesJar"])
            }
        }
    }
}
