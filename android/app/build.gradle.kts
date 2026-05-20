// app/build.gradle.kts — app module build configuration and dependencies
//
// Key settings:
//   minSdk 26 (Android 8.0) — minimum required for BLE connection parameter
//     negotiation used to prevent 0x13 disconnects (see firmware/CLAUDE.md bug #4)
//   compileSdk / targetSdk 34 — target Android 14
//   isMinifyEnabled = false — ProGuard/R8 obfuscation disabled; enable when
//     preparing a release build to reduce APK size
//
// Compose compiler compatibility:
//   kotlinCompilerExtensionVersion "1.5.8" must match Kotlin 1.9.22
//   If you upgrade Kotlin, check the compatibility table before changing this:
//   https://developer.android.com/jetpack/androidx/releases/compose-kotlin
//
// Dependencies — what each one does:
//   core-ktx                  Kotlin extensions for Android core APIs
//   lifecycle-runtime-ktx     viewModelScope, coroutine lifecycle integration
//   lifecycle-viewmodel-compose  viewModel() Compose integration for MainViewModel
//   activity-compose          setContent{} and ComponentActivity Compose support
//   compose-bom               Bill of Materials — pins all compose/* versions together
//   compose ui / ui-graphics  core Compose UI rendering
//   ui-tooling-preview         @Preview annotation support in Android Studio
//   material3                 Material Design 3 components (used throughout MainScreen)
//   ui-tooling (debug)        layout inspector support
//   ui-test-manifest (debug)  test runner manifest entries
//
// Related:
//   android/build.gradle.kts        — plugin version declarations
//   android/settings.gradle.kts     — project name and module list
//   android/notes/setup.md          — how to open and build in Android Studio

plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.andrewscrap.refineps"
    compileSdk = 34

    defaultConfig {
        applicationId = "com.andrewscrap.refineps"
        minSdk = 26
        targetSdk = 34
        versionCode = 1
        versionName = "1.0"
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(
                getDefaultProguardFile("proguard-android-optimize.txt"),
                "proguard-rules.pro"
            )
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_1_8
        targetCompatibility = JavaVersion.VERSION_1_8
    }

    kotlinOptions {
        jvmTarget = "1.8"
    }

    buildFeatures {
        compose = true
    }

    composeOptions {
        kotlinCompilerExtensionVersion = "1.5.8"
    }
}

dependencies {
    implementation("androidx.core:core-ktx:1.12.0")
    implementation("androidx.lifecycle:lifecycle-runtime-ktx:2.7.0")
    implementation("androidx.lifecycle:lifecycle-viewmodel-compose:2.7.0")
    implementation("androidx.activity:activity-compose:1.8.2")

    implementation(platform("androidx.compose:compose-bom:2024.02.00"))
    implementation("androidx.compose.ui:ui")
    implementation("androidx.compose.ui:ui-graphics")
    implementation("androidx.compose.ui:ui-tooling-preview")
    implementation("androidx.compose.material3:material3")

    debugImplementation("androidx.compose.ui:ui-tooling")
    debugImplementation("androidx.compose.ui:ui-test-manifest")
}
