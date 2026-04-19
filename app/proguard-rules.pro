# RefinePS ProGuard Rules
# Keep BLE callback methods
-keepclassmembers class * extends android.bluetooth.BluetoothGattCallback {
    *;
}
