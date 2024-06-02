package com.stephanduechtel.multitrackplayer

import android.app.Application
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.BorderStroke
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.PaddingValues
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.statusBarsPadding
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.material.Button
import androidx.compose.material.ButtonDefaults
import androidx.compose.material.Scaffold
import androidx.compose.material.Slider
import androidx.compose.material.Text
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import com.stephanduechtel.multitrackplayer.ui.theme.MultitrackplayerTheme
import androidx.lifecycle.viewmodel.compose.viewModel
import androidx.compose.runtime.*

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            MultitrackplayerTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    Greeting(
                        name = "Android",
                        modifier = Modifier.padding(innerPadding)
                    )
                }
            }
        }
    }
}

@Composable
fun Greeting(name: String, modifier: Modifier = Modifier) {

    val appcontext = LocalContext.current.applicationContext as Application
    val viewModel: PlayerViewModel = viewModel(
        factory = PlayerViewModelFactory(appcontext)
    )

    Box( // Changed to Box here as well, considering it's filling available space
        modifier = Modifier
            .fillMaxWidth()
            .background(Color.Black)
            .statusBarsPadding()
            .padding(horizontal = 16.dp)
    ) {
        Column {
            // Upper column to take up all available space
            Column(
                modifier = Modifier
                    .weight(1f) // Takes up all available space
                    .fillMaxHeight()
            ) {
                // Content of the upper column
                Box(modifier = Modifier.fillMaxSize()) {
                    // Adding 5 sliders here
                    Column(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(16.dp),
                        verticalArrangement = Arrangement.spacedBy(16.dp)
                    ) {
                        VolumeSlider(
                            label = "Player 1",
                            volume = viewModel.player1Volume,
                            onVolumeChange = { viewModel.updatePlayer1Volume(it) }
                        )
                        VolumeSlider(
                            label = "Player 2",
                            volume = viewModel.player2Volume,
                            onVolumeChange = { viewModel.updatePlayer2Volume(it) }
                        )
                        // Add 3 more sliders as placeholders for future controls
                        VolumeSlider(
                            label = "Player 3",
                            volume = viewModel.player3Volume,
                            onVolumeChange = { viewModel.updatePlayer3Volume(it) }
                        )
                        VolumeSlider(
                            label = "Player 4",
                            volume = viewModel.player4Volume,
                            onVolumeChange = { viewModel.updatePlayer4Volume(it) }
                        )
                        VolumeSlider(
                            label = "Player 5",
                            volume = viewModel.player5Volume,
                            onVolumeChange = { viewModel.updatePlayer5Volume(it) }
                        )
                    }
                }
            }

            // Bottom column with fixed height
            Column(
                modifier = Modifier
                    .height(200.dp) // Fixed height
            ) {
                // Content of the bottom column
                Box(modifier = Modifier
                    .fillMaxSize()) {
                    Column(
                        modifier = Modifier
                            .fillMaxWidth()
                            .padding(horizontal = 16.dp),
                        horizontalAlignment = Alignment.CenterHorizontally
                    ) {
                        // Seeker with labels
                        Seeker(
                            currentTimeInSeconds = viewModel.currentTimeInSeconds,
                            totalLengthInSeconds = viewModel.totalLengthInSeconds,
                            onSeek = { newTime ->
                                viewModel.seekTo(newTime)
                            },
                            onSeekStart = {
                                viewModel.onSeekStart()
                            },
                            onSeekEnd = {
                                viewModel.onSeekEnd()
                            }
                        )

                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(horizontal = 5.dp),
                            horizontalArrangement = Arrangement.SpaceBetween,
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            TextButton(label = "Tempo Down", onClick = { viewModel.tempoDown() })
                            TextButton(label = "Tempo Up", onClick = { viewModel.tempoUp() })
                            TextButton(label = "Play/Pause", onClick = { viewModel.playPause() })
                            TextButton(label = "Pitch Down", onClick = { viewModel.pitchDown() })
                            TextButton(label = "Pitch Up", onClick = { viewModel.pitchUp() })
                        }

                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .padding(horizontal = 5.dp),
                            horizontalArrangement = Arrangement.SpaceBetween,
                            verticalAlignment = Alignment.CenterVertically
                        ) {
                            TextButton(label = "Loop", onClick = { viewModel.loop() })
                        }

                    }
                }
            }
        }

    }
}

@Composable
fun Seeker(
    currentTimeInSeconds: Float,
    totalLengthInSeconds: Float,
    onSeek: (Float) -> Unit,
    onSeekStart: () -> Unit,
    onSeekEnd: () -> Unit
) {
    var sliderPosition by remember { mutableStateOf(currentTimeInSeconds) }
    val isDragging = remember { mutableStateOf(false) }

    LaunchedEffect(currentTimeInSeconds) {
        if (!isDragging.value) {
            sliderPosition = currentTimeInSeconds
        }
    }

    Column(
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center,
        modifier = Modifier.fillMaxWidth()
    ) {
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.SpaceBetween,
            verticalAlignment = Alignment.CenterVertically
        ) {
            Text(
                text = formatTime(currentTimeInSeconds),
                color = Color.White
            )
            Text(
                text = formatTime(totalLengthInSeconds),
                color = Color.White
            )
        }
        Slider(
            value = sliderPosition,
            onValueChange = { newValue ->
                sliderPosition = newValue
                onSeek(newValue)
                if (!isDragging.value) {
                    isDragging.value = true
                    onSeekStart()
                }
            },
            onValueChangeFinished = {
                onSeekEnd()
                isDragging.value = false
            },
            valueRange = 0f..totalLengthInSeconds,
            modifier = Modifier.fillMaxWidth()
        )
    }
}

private fun formatTime(seconds: Float): String {
    val minutes = seconds.toInt() / 60
    val secs = seconds.toInt() % 60
    return String.format("%d:%02d", minutes, secs)
}

@Composable
fun VolumeSlider(label: String, volume: Float, onVolumeChange: (Float) -> Unit) {
    Column {
        Text(text = label, color = Color.White)
        Slider(
            value = volume,
            onValueChange = onVolumeChange,
            valueRange = 0f..1f,
            modifier = Modifier.fillMaxWidth()
        )
    }
}

@Composable
fun TextButton(label: String, onClick: () -> Unit) {
    Button(
        onClick = onClick,
        shape = CircleShape, // Make the button circular
        colors = ButtonDefaults.buttonColors(backgroundColor = Color.Transparent), // Transparent button background
        border = BorderStroke(1.dp, Color.White), // Thin white border
        contentPadding = PaddingValues(),
        modifier = Modifier
            .width(60.dp)
            .height(60.dp)
    ) {
        Box(
            contentAlignment = Alignment.Center, // Center the content inside the box
            modifier = Modifier.fillMaxSize()
        ) {
            Text(
                text = label,
                color = Color.White, // Text color
                textAlign = TextAlign.Center, // Center the text if it wraps to multiple lines
                modifier = Modifier
                    .fillMaxWidth()
                    .align(Alignment.Center) // Center the text inside the box
            )
        }
    }
}

@Preview(showBackground = true)
@Composable
fun GreetingPreview() {
    MultitrackplayerTheme {
        Greeting("Android")
    }
}