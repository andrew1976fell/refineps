// build.gradle.kts (root) — project-level plugin version declarations
//
// Declares plugin versions centrally with "apply false" — the version is
// pinned here but the plugin is NOT applied to the root project. Each module
// (e.g. app/) applies the plugin independently without repeating the version.
//
// When upgrading AGP or Kotlin, change the versions here only.
// The Compose compiler extension version in app/build.gradle.kts must stay
// in sync with the Kotlin version — check the compatibility map at:
// https://developer.android.com/jetpack/androidx/releases/compose-kotlin
//
// Current versions:
//   Android Gradle Plugin (AGP): 8.13.2
//   Kotlin:                      1.9.22
//   Gradle wrapper:              8.13  (see gradle/wrapper/gradle-wrapper.properties)
//
// Related:
//   android/app/build.gradle.kts            — app module config and dependencies
//   android/gradle/wrapper/gradle-wrapper.properties — Gradle version

plugins {
    id("com.android.application") version "8.13.2" apply false
    id("org.jetbrains.kotlin.android") version "1.9.22" apply false
}
