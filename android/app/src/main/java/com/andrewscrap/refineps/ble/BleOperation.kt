/*
 * BleOperation.kt — sealed class for the BleManager operation queue
 *
 * Each subclass represents one GATT operation. BleManager enqueues these
 * and executes them one at a time — the Android GATT stack requires
 * serialization or operations are silently dropped.
 *
 * WriteCharacteristic overrides equals/hashCode because it holds a ByteArray,
 * which uses reference equality by default in Kotlin/JVM — without the
 * override, two identical write operations would compare as unequal.
 *
 * Related: ble/BleManager.kt
 */
package com.andrewscrap.refineps.ble

import android.bluetooth.BluetoothDevice
import java.util.UUID

sealed class BleOperation {
    data class Connect(val device: BluetoothDevice) : BleOperation()
    data object DiscoverServices : BleOperation()
    data class RequestMtu(val mtu: Int) : BleOperation()
    data class EnableNotifications(val serviceUuid: UUID, val charUuid: UUID) : BleOperation()
    data class WriteCharacteristic(val serviceUuid: UUID, val charUuid: UUID, val data: ByteArray) : BleOperation() {
        override fun equals(other: Any?): Boolean {
            if (this === other) return true
            if (other !is WriteCharacteristic) return false
            return serviceUuid == other.serviceUuid && charUuid == other.charUuid && data.contentEquals(other.data)
        }
        override fun hashCode(): Int {
            var result = serviceUuid.hashCode()
            result = 31 * result + charUuid.hashCode()
            result = 31 * result + data.contentHashCode()
            return result
        }
    }
    data object Disconnect : BleOperation()
}
