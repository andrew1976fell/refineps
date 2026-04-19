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
