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
