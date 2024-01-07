@file:Suppress("UnstableApiUsage")

plugins {
    id("com.android.library")
    id("org.jetbrains.kotlin.android")
    id("maven-publish")
}

afterEvaluate {
    publishing {
        publications {
            create<MavenPublication>("mavenJava") {
                groupId = "com.github.t8rin"
                artifactId = "bitmap-scaler"
                version = "1.1.0"
                from(components["release"])
            }
        }
    }
}

android {
    namespace = "com.t8rin.bitmapscaler"
    compileSdk = 34

    defaultConfig {
        minSdk = 14

        externalNativeBuild {
            cmake {
                ndkVersion = "26.1.10909125"
                cppFlags.add("-std=c++20")
                abiFilters += setOf("armeabi-v7a", "arm64-v8a", "x86_64", "x86")
            }
        }

        publishing {
            singleVariant("release") {
                withSourcesJar()
                withJavadocJar()
            }
        }
    }

    sourceSets.named("main") {
        this.jniLibs {
            this.srcDir("src/main/cpp/lib")
        }
    }

    externalNativeBuild {
        cmake {
            path("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions {
        jvmTarget = "17"
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.12.0")
}