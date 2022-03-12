plugins {
    alias(deps.plugins.android.library)
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
        setFlavorDimensions(mutableListOf("loader"))
        create("riru") {
            dimension = "loader"

            externalNativeBuild {
                cmake {
                    arguments("-DLOADER:STRING=riru")
                }
            }
        }
        create("zygisk") {
            dimension = "loader"

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
    publishing {
        multipleVariants("all") {
            withSourcesJar()
            includeBuildTypeValues("debug", "release")
            includeFlavorDimensionAndValues("loader", "riru", "zygisk")
        }
    }
}

dependencies {
    val riruImplementation by configurations

    compileOnly(deps.androidx.annotation)

    riruImplementation(deps.riru.runtime)
}

afterEvaluate {
    publishing {
        publications {
            named(project.name, MavenPublication::class) {
                from(components["all"])
            }
        }
    }
}
