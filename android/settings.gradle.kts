// settings.gradle.kts — project settings: name, modules, and repository sources
//
// Evaluated before any build.gradle.kts file. Sets:
//   rootProject.name  the project name shown in Android Studio ("RefinePS")
//   include(":app")   registers the app/ directory as the only module
//
// FAIL_ON_PROJECT_REPOS prevents individual modules from declaring their own
// repositories — all dependency sources must be declared here centrally.
// This catches accidental repo drift across modules.
//
// To add a new module (e.g. a library): add include(":libraryname") and
// create the corresponding directory with its own build.gradle.kts.
//
// Related:
//   android/build.gradle.kts    — plugin version declarations
//   android/app/build.gradle.kts — app module config

pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}

dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentral()
    }
}

rootProject.name = "RefinePS"
include(":app")
