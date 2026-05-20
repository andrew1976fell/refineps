/*
 * MainViewModel.kt — UI state and BLE command logic
 *
 * Owns BleManager and exposes a single UiState StateFlow to the UI.
 * Uses AndroidViewModel (not ViewModel) because BleManager needs an
 * Application context that outlives the Activity.
 *
 * UiState fields:
 *   connectionState  — DISCONNECTED / SCANNING / CONNECTING / CONNECTED
 *   deviceName       — name of the connected device ("RefinePS")
 *   dutyA            — high-phase duty cycle (0–100%), bound to slider
 *   dutyB            — low-phase duty cycle (0–100%), bound to slider
 *   lastTelemetry    — most recent telemetry JSON (pretty-printed)
 *   logLines         — last 20 log messages (auto-truncated)
 *
 * Current limitations:
 *   - sendCommand is hardcoded to CH1 (ch=1) — multi-channel not yet wired
 *   - sendStatusCommand exists but is not called from the UI
 *   - freq_carrier and freq_switch are not exposed — firmware defaults apply
 *
 * Related:
 *   ble/BleManager.kt           — BLE scan, connect, send, receive
 *   ui/MainScreen.kt            — consumes UiState, fires callbacks
 *   firmware/refine_schema_v1.1.md — full command/response protocol
 */
package com.andrewscrap.refineps

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.andrewscrap.refineps.ble.BleEvent
import com.andrewscrap.refineps.ble.BleManager
import com.andrewscrap.refineps.ble.ConnectionState
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch
import org.json.JSONObject

data class UiState(
    val connectionState: ConnectionState = ConnectionState.DISCONNECTED,
    val deviceName: String? = null,
    val dutyA: Int = 50,
    val dutyB: Int = 50,
    val lastTelemetry: String = "",
    val logLines: List<String> = emptyList()
)

class MainViewModel(application: Application) : AndroidViewModel(application) {

    private val bleManager = BleManager(application)

    private val _uiState = MutableStateFlow(UiState())
    val uiState: StateFlow<UiState> = _uiState

    init {
        viewModelScope.launch {
            bleManager.connectionState.collect { state ->
                _uiState.update { it.copy(connectionState = state) }
            }
        }

        viewModelScope.launch {
            bleManager.deviceName.collect { name ->
                _uiState.update { it.copy(deviceName = name) }
            }
        }

        viewModelScope.launch {
            bleManager.events.collect { event ->
                when (event) {
                    is BleEvent.Log -> addLog(event.message)
                    is BleEvent.Telemetry -> handleTelemetry(event.json)
                    is BleEvent.Error -> addLog("ERROR: ${event.message}")
                }
            }
        }
    }

    fun startScan() {
        bleManager.startScan()
    }

    fun disconnect() {
        bleManager.disconnect()
    }

    fun setDutyA(value: Int) {
        _uiState.update { it.copy(dutyA = value) }
    }

    fun setDutyB(value: Int) {
        _uiState.update { it.copy(dutyB = value) }
    }

    fun sendCommand() {
        val state = _uiState.value
        val json = JSONObject().apply {
            put("magic", "refine")
            put("cmd", "set")
            put("ch", 1)
            put("duty_a", state.dutyA)
            put("duty_b", state.dutyB)
        }.toString()

        addLog("Sending: $json")
        bleManager.sendCommand(json)
    }

    fun sendStatusCommand() {
        val json = """{"magic":"refine","cmd":"status"}"""
        addLog("Sending: $json")
        bleManager.sendCommand(json)
    }

    private fun handleTelemetry(json: String) {
        val prettyJson = try {
            JSONObject(json).toString(2)
        } catch (e: Exception) {
            json
        }
        _uiState.update { it.copy(lastTelemetry = prettyJson) }
    }

    private fun addLog(message: String) {
        _uiState.update { current ->
            val newLogs = (current.logLines + message).takeLast(20)
            current.copy(logLines = newLogs)
        }
    }
}
