plugins {
    id("com.android.library")
    `maven-publish`
}

android {
    compileSdk = 31
    ndkVersion = "23.0.7599858"

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
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
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
