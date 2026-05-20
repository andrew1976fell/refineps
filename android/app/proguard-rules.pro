# proguard-rules.pro — ProGuard/R8 rules for release builds
#
# Only applies when isMinifyEnabled = true in app/build.gradle.kts.
# Currently isMinifyEnabled = false, so these rules have no effect —
# but they are ready for when minification is enabled for release.
#
# Rule: keep all members of BluetoothGattCallback subclasses.
# Without this, R8 strips the callback methods (onConnectionStateChange,
# onServicesDiscovered, onCharacteristicChanged etc.) because they appear
# unreachable at compile time — they are called by the Android BLE stack
# via reflection, not directly from app code.
#
# If you add more classes called via reflection (e.g. serialization,
# dependency injection), add -keep rules for them here.
#
# Related:
#   android/app/build.gradle.kts    — isMinifyEnabled setting
#   android/app/src/main/java/.../ble/BleManager.kt — the callback class

# Keep BLE callback methods — called by Android BLE stack via reflection
-keepclassmembers class * extends android.bluetooth.BluetoothGattCallback {
    *;
}
