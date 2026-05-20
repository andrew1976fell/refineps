/*
 * BleManager.kt — BLE scan, connect, send, and receive
 *
 * Scans for a device named "RefinePS", connects, and manages the full
 * GATT lifecycle. All GATT operations are serialized through an operation
 * queue — the Android GATT stack is not thread-safe and will silently drop
 * concurrent calls. Each operation calls completeOperation() when done,
 * which triggers the next item in the queue.
 *
 * Connection sequence (via operation queue):
 *   Connect → DiscoverServices → RequestMtu(512) → EnableNotifications → CONNECTED
 *
 * Sending: sendCommand() appends "\n", chunks into (MTU-3)-byte pieces,
 *   enqueues one WriteCharacteristic per chunk.
 *
 * Receiving: handleNotification() appends to receiveBuffer and emits a
 *   BleEvent.Telemetry for each newline-terminated JSON message. The firmware
 *   appends "\n" to every response, so each response arrives as one event.
 *
 * API compatibility:
 *   onCharacteristicChanged has two overrides — deprecated (API < 33) and
 *   current (API >= 33). Both delegate to handleNotification(). The
 *   @SuppressLint("MissingPermission") annotation is safe here because
 *   MainActivity checks permissions before any BLE call.
 *
 * UUIDs — must match firmware constants in bt_serial.c:
 *   Service  0x00FF   Write char 0xFF01   Notify char 0xFF02
 *
 * Related:
 *   ble/BleOperation.kt         — operation queue sealed class
 *   MainViewModel.kt            — consumes connectionState, deviceName, events
 *   android/notes/ble.md        — BLE implementation notes and known issues
 *   firmware/CLAUDE.md          — firmware-side UUID definitions
 *   firmware/refine_schema_v1.1.md — full protocol reference
 */
package com.andrewscrap.refineps.ble

import android.annotation.SuppressLint
import android.bluetooth.*
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.os.Build
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.flow.StateFlow
import java.util.UUID
import java.util.concurrent.ConcurrentLinkedQueue

enum class ConnectionState {
    DISCONNECTED,
    SCANNING,
    CONNECTING,
    CONNECTED
}

sealed class BleEvent {
    data class Log(val message: String) : BleEvent()
    data class Telemetry(val json: String) : BleEvent()
    data class Error(val message: String) : BleEvent()
}

@SuppressLint("MissingPermission")
class BleManager(private val context: Context) {

    companion object {
        const val DEVICE_NAME = "RefinePS"
        val SERVICE_UUID: UUID = UUID.fromString("000000FF-0000-1000-8000-00805F9B34FB")
        val WRITE_CHAR_UUID: UUID = UUID.fromString("0000FF01-0000-1000-8000-00805F9B34FB")
        val NOTIFY_CHAR_UUID: UUID = UUID.fromString("0000FF02-0000-1000-8000-00805F9B34FB")
        val CCCD_UUID: UUID = UUID.fromString("00002902-0000-1000-8000-00805F9B34FB")
        private const val MTU_SIZE = 512
    }

    private val bluetoothManager = context.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
    private val bluetoothAdapter: BluetoothAdapter? = bluetoothManager.adapter
    private val scanner = bluetoothAdapter?.bluetoothLeScanner

    private var gatt: BluetoothGatt? = null
    private val operationQueue = ConcurrentLinkedQueue<BleOperation>()
    private var isOperationInProgress = false
    private var negotiatedMtu = 23  // BLE default; updated on MTU negotiation
    private val receiveBuffer = StringBuilder()

    private val _connectionState = MutableStateFlow(ConnectionState.DISCONNECTED)
    val connectionState: StateFlow<ConnectionState> = _connectionState

    private val _deviceName = MutableStateFlow<String?>(null)
    val deviceName: StateFlow<String?> = _deviceName

    private val _events = MutableSharedFlow<BleEvent>(extraBufferCapacity = 64)
    val events: SharedFlow<BleEvent> = _events

    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val device = result.device
            val name = device.name ?: return
            if (name == DEVICE_NAME) {
                stopScan()
                _deviceName.value = name
                emitLog("Found device: $name")
                enqueueOperation(BleOperation.Connect(device))
            }
        }

        override fun onScanFailed(errorCode: Int) {
            emitLog("Scan failed: $errorCode")
            _connectionState.value = ConnectionState.DISCONNECTED
        }
    }

    private val gattCallback = object : BluetoothGattCallback() {

        override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                when (newState) {
                    BluetoothProfile.STATE_CONNECTED -> {
                        emitLog("Connected to GATT server")
                        this@BleManager.gatt = gatt
                        _connectionState.value = ConnectionState.CONNECTING
                        completeOperation()
                        enqueueOperation(BleOperation.DiscoverServices)
                    }
                    BluetoothProfile.STATE_DISCONNECTED -> {
                        emitLog("Disconnected from GATT server")
                        cleanup()
                    }
                }
            } else {
                emitLog("Connection state change failed: status=$status")
                cleanup()
            }
        }

        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                emitLog("Services discovered")
                completeOperation()
                enqueueOperation(BleOperation.RequestMtu(MTU_SIZE))
            } else {
                emitLog("Service discovery failed: $status")
                completeOperation()
            }
        }

        override fun onMtuChanged(gatt: BluetoothGatt, mtu: Int, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                negotiatedMtu = mtu
                emitLog("MTU changed to $mtu (payload ${mtu - 3} bytes)")
            } else {
                emitLog("MTU change failed: $status")
            }
            completeOperation()
            enqueueOperation(BleOperation.EnableNotifications(SERVICE_UUID, NOTIFY_CHAR_UUID))
        }

        override fun onDescriptorWrite(gatt: BluetoothGatt, descriptor: BluetoothGattDescriptor, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                emitLog("Notifications enabled")
                _connectionState.value = ConnectionState.CONNECTED
            } else {
                emitLog("Descriptor write failed: $status")
            }
            completeOperation()
        }

        override fun onCharacteristicWrite(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic, status: Int) {
            if (status == BluetoothGatt.GATT_SUCCESS) {
                emitLog("Command sent successfully")
            } else {
                emitLog("Characteristic write failed: $status")
            }
            completeOperation()
        }

        // Deprecated callback for API < 33
        @Deprecated("Deprecated in API 33")
        @Suppress("DEPRECATION")
        override fun onCharacteristicChanged(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic) {
            if (Build.VERSION.SDK_INT < 33) {
                handleNotification(characteristic.value)
            }
        }

        // New callback for API >= 33
        override fun onCharacteristicChanged(gatt: BluetoothGatt, characteristic: BluetoothGattCharacteristic, value: ByteArray) {
            handleNotification(value)
        }
    }

    private fun handleNotification(data: ByteArray) {
        receiveBuffer.append(String(data, Charsets.UTF_8))
        var newlineIdx = receiveBuffer.indexOf('\n')
        while (newlineIdx >= 0) {
            val json = receiveBuffer.substring(0, newlineIdx).trim()
            receiveBuffer.delete(0, newlineIdx + 1)
            if (json.isNotEmpty()) _events.tryEmit(BleEvent.Telemetry(json))
            newlineIdx = receiveBuffer.indexOf('\n')
        }
    }

    fun startScan() {
        if (scanner == null) {
            emitLog("Bluetooth LE scanner not available")
            return
        }
        if (_connectionState.value != ConnectionState.DISCONNECTED) {
            emitLog("Already scanning or connected")
            return
        }

        _connectionState.value = ConnectionState.SCANNING
        emitLog("Starting scan for $DEVICE_NAME...")

        val settings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()

        scanner.startScan(null, settings, scanCallback)
    }

    fun stopScan() {
        scanner?.stopScan(scanCallback)
        if (_connectionState.value == ConnectionState.SCANNING) {
            _connectionState.value = ConnectionState.DISCONNECTED
        }
    }

    fun disconnect() {
        enqueueOperation(BleOperation.Disconnect)
    }

    fun sendCommand(json: String) {
        val data = (json + "\n").toByteArray(Charsets.UTF_8)
        val chunkSize = negotiatedMtu - 3
        data.toList().chunked(chunkSize).forEach { chunk ->
            enqueueOperation(BleOperation.WriteCharacteristic(SERVICE_UUID, WRITE_CHAR_UUID, chunk.toByteArray()))
        }
    }

    private fun enqueueOperation(operation: BleOperation) {
        operationQueue.add(operation)
        processNextOperation()
    }

    @Synchronized
    private fun processNextOperation() {
        if (isOperationInProgress) return
        val operation = operationQueue.poll() ?: return

        isOperationInProgress = true

        when (operation) {
            is BleOperation.Connect -> {
                _connectionState.value = ConnectionState.CONNECTING
                emitLog("Connecting to ${operation.device.name}...")
                operation.device.connectGatt(context, false, gattCallback, BluetoothDevice.TRANSPORT_LE)
            }

            is BleOperation.DiscoverServices -> {
                gatt?.discoverServices() ?: completeOperation()
            }

            is BleOperation.RequestMtu -> {
                gatt?.requestMtu(operation.mtu) ?: completeOperation()
            }

            is BleOperation.EnableNotifications -> {
                val service = gatt?.getService(operation.serviceUuid)
                val characteristic = service?.getCharacteristic(operation.charUuid)
                if (characteristic == null) {
                    emitLog("Notify characteristic not found")
                    completeOperation()
                    return
                }

                gatt?.setCharacteristicNotification(characteristic, true)

                val descriptor = characteristic.getDescriptor(CCCD_UUID)
                if (descriptor == null) {
                    emitLog("CCCD descriptor not found")
                    completeOperation()
                    return
                }

                if (Build.VERSION.SDK_INT >= 33) {
                    gatt?.writeDescriptor(descriptor, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)
                } else {
                    @Suppress("DEPRECATION")
                    descriptor.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                    @Suppress("DEPRECATION")
                    gatt?.writeDescriptor(descriptor)
                }
            }

            is BleOperation.WriteCharacteristic -> {
                val service = gatt?.getService(operation.serviceUuid)
                val characteristic = service?.getCharacteristic(operation.charUuid)
                if (characteristic == null) {
                    emitLog("Write characteristic not found")
                    completeOperation()
                    return
                }

                if (Build.VERSION.SDK_INT >= 33) {
                    gatt?.writeCharacteristic(
                        characteristic,
                        operation.data,
                        BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
                    )
                } else {
                    @Suppress("DEPRECATION")
                    characteristic.value = operation.data
                    characteristic.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
                    @Suppress("DEPRECATION")
                    gatt?.writeCharacteristic(characteristic)
                }
            }

            is BleOperation.Disconnect -> {
                gatt?.disconnect()
                completeOperation()
            }
        }
    }

    @Synchronized
    private fun completeOperation() {
        isOperationInProgress = false
        processNextOperation()
    }

    private fun cleanup() {
        operationQueue.clear()
        isOperationInProgress = false
        gatt?.close()
        gatt = null
        negotiatedMtu = 23
        receiveBuffer.clear()
        _connectionState.value = ConnectionState.DISCONNECTED
        _deviceName.value = null
    }

    private fun emitLog(message: String) {
        _events.tryEmit(BleEvent.Log(message))
    }
}
