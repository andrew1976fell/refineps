package com.andrewscrap.refineps.ui

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.*
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.lazy.rememberLazyListState
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.andrewscrap.refineps.UiState
import com.andrewscrap.refineps.ble.ConnectionState

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun MainScreen(
    uiState: UiState,
    onScanClick: () -> Unit,
    onDisconnectClick: () -> Unit,
    onDutyAChange: (Int) -> Unit,
    onDutyBChange: (Int) -> Unit,
    onSendClick: () -> Unit,
    modifier: Modifier = Modifier
) {
    Scaffold(
        topBar = {
            TopAppBar(
                title = { Text("RefinePS") },
                actions = {
                    StatusChip(uiState.connectionState, uiState.deviceName)
                }
            )
        }
    ) { padding ->
        Column(
            modifier = modifier
                .fillMaxSize()
                .padding(padding)
                .padding(16.dp)
                .verticalScroll(rememberScrollState()),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            // Connection button
            ConnectionButton(
                connectionState = uiState.connectionState,
                onScanClick = onScanClick,
                onDisconnectClick = onDisconnectClick
            )

            // Channel 1 Controls
            ChannelControls(
                dutyA = uiState.dutyA,
                dutyB = uiState.dutyB,
                enabled = uiState.connectionState == ConnectionState.CONNECTED,
                onDutyAChange = onDutyAChange,
                onDutyBChange = onDutyBChange,
                onSendClick = onSendClick
            )

            // Telemetry display
            TelemetryCard(telemetry = uiState.lastTelemetry)

            // Log window
            LogWindow(logs = uiState.logLines)
        }
    }
}

@Composable
private fun StatusChip(connectionState: ConnectionState, deviceName: String?) {
    val (text, color) = when (connectionState) {
        ConnectionState.DISCONNECTED -> "Disconnected" to MaterialTheme.colorScheme.error
        ConnectionState.SCANNING -> "Scanning..." to MaterialTheme.colorScheme.tertiary
        ConnectionState.CONNECTING -> "Connecting..." to MaterialTheme.colorScheme.tertiary
        ConnectionState.CONNECTED -> (deviceName ?: "Connected") to MaterialTheme.colorScheme.primary
    }

    SuggestionChip(
        onClick = {},
        label = { Text(text) },
        colors = SuggestionChipDefaults.suggestionChipColors(
            containerColor = color.copy(alpha = 0.2f),
            labelColor = color
        ),
        modifier = Modifier.padding(end = 8.dp)
    )
}

@Composable
private fun ConnectionButton(
    connectionState: ConnectionState,
    onScanClick: () -> Unit,
    onDisconnectClick: () -> Unit
) {
    when (connectionState) {
        ConnectionState.DISCONNECTED -> {
            Button(
                onClick = onScanClick,
                modifier = Modifier.fillMaxWidth()
            ) {
                Text("Scan & Connect")
            }
        }
        ConnectionState.SCANNING, ConnectionState.CONNECTING -> {
            OutlinedButton(
                onClick = onDisconnectClick,
                modifier = Modifier.fillMaxWidth()
            ) {
                CircularProgressIndicator(
                    modifier = Modifier.size(16.dp),
                    strokeWidth = 2.dp
                )
                Spacer(Modifier.width(8.dp))
                Text("Cancel")
            }
        }
        ConnectionState.CONNECTED -> {
            OutlinedButton(
                onClick = onDisconnectClick,
                modifier = Modifier.fillMaxWidth(),
                colors = ButtonDefaults.outlinedButtonColors(
                    contentColor = MaterialTheme.colorScheme.error
                )
            ) {
                Text("Disconnect")
            }
        }
    }
}

@Composable
private fun ChannelControls(
    dutyA: Int,
    dutyB: Int,
    enabled: Boolean,
    onDutyAChange: (Int) -> Unit,
    onDutyBChange: (Int) -> Unit,
    onSendClick: () -> Unit
) {
    Card(
        modifier = Modifier.fillMaxWidth()
    ) {
        Column(
            modifier = Modifier.padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(16.dp)
        ) {
            Text(
                text = "Channel 1",
                style = MaterialTheme.typography.titleMedium
            )

            // Power % slider
            Column {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Text("Power %")
                    Text("$dutyA%")
                }
                Slider(
                    value = dutyA.toFloat(),
                    onValueChange = { onDutyAChange(it.toInt()) },
                    valueRange = 0f..100f,
                    enabled = enabled
                )
            }

            // Pulse low % slider
            Column {
                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween
                ) {
                    Text("Pulse low %")
                    Text("$dutyB%")
                }
                Slider(
                    value = dutyB.toFloat(),
                    onValueChange = { onDutyBChange(it.toInt()) },
                    valueRange = 0f..100f,
                    enabled = enabled
                )
            }

            Button(
                onClick = onSendClick,
                enabled = enabled,
                modifier = Modifier.fillMaxWidth()
            ) {
                Text("Send")
            }
        }
    }
}

@Composable
private fun TelemetryCard(telemetry: String) {
    Card(
        modifier = Modifier.fillMaxWidth()
    ) {
        Column(
            modifier = Modifier.padding(16.dp)
        ) {
            Text(
                text = "Telemetry",
                style = MaterialTheme.typography.titleMedium
            )
            Spacer(Modifier.height(8.dp))
            if (telemetry.isNotEmpty()) {
                Text(
                    text = telemetry,
                    fontFamily = FontFamily.Monospace,
                    fontSize = 12.sp,
                    modifier = Modifier
                        .fillMaxWidth()
                        .background(
                            MaterialTheme.colorScheme.surfaceVariant,
                            MaterialTheme.shapes.small
                        )
                        .padding(8.dp)
                )
            } else {
                Text(
                    text = "No telemetry received",
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                    fontSize = 12.sp
                )
            }
        }
    }
}

@Composable
private fun LogWindow(logs: List<String>) {
    Card(
        modifier = Modifier
            .fillMaxWidth()
            .height(200.dp)
    ) {
        Column(
            modifier = Modifier.padding(16.dp)
        ) {
            Text(
                text = "Log",
                style = MaterialTheme.typography.titleMedium
            )
            Spacer(Modifier.height(8.dp))

            val listState = rememberLazyListState()

            LaunchedEffect(logs.size) {
                if (logs.isNotEmpty()) {
                    listState.animateScrollToItem(logs.size - 1)
                }
            }

            LazyColumn(
                state = listState,
                modifier = Modifier
                    .fillMaxSize()
                    .background(
                        MaterialTheme.colorScheme.surfaceVariant,
                        MaterialTheme.shapes.small
                    )
                    .padding(8.dp)
            ) {
                items(logs) { log ->
                    Text(
                        text = log,
                        fontFamily = FontFamily.Monospace,
                        fontSize = 11.sp,
                        lineHeight = 14.sp
                    )
                }
            }
        }
    }
}
