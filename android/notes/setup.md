# Android App Setup

## Opening the project in Android Studio

1. Clone the repo (or `git pull` if already cloned):
   ```
   git clone https://github.com/andrew1976fell/refineps.git
   ```
2. Open Android Studio
3. **File → Open** → navigate into the repo, select the `android/` subfolder
   - Do NOT open the repo root — Gradle will not find the project
4. Let Gradle sync finish
5. Connect an Android device or start an emulator
6. Run the `:app` configuration

## First-time machine setup

- Install Android Studio (includes JDK and Android SDK)
- No other tools needed — Gradle downloads its own dependencies
- Min Android version on test device: Android 8.0 (API 26)

## Build details

| Setting | Value |
|---------|-------|
| compileSdk | 34 |
| minSdk | 26 |
| Kotlin | via `org.jetbrains.kotlin.android` plugin |
| Compose compiler | 1.5.8 |
| Gradle wrapper | see `gradle/wrapper/gradle-wrapper.properties` |

## If Android Studio complains after a `git pull`

This happens if the project was previously opened from the old repo root location.
Fix: **File → Open** again and re-point to `android/` — do not just reopen the last project.
