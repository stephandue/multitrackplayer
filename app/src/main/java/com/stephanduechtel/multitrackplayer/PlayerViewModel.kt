package com.stephanduechtel.multitrackplayer

import android.app.Application
import androidx.annotation.OptIn
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.media3.common.util.UnstableApi
import com.stephanduechtel.fiveloop.ExoPlayerManager

class PlayerViewModel(application: Application): AndroidViewModel(application) {

    var player1Volume by mutableStateOf(1f)
    var player2Volume by mutableStateOf(1f)
    var player3Volume by mutableStateOf(1f)
    var player4Volume by mutableStateOf(1f)
    var player5Volume by mutableStateOf(1f)

    init {
        println("init PlayerViewModel")
        ExoPlayerManager.initialize(getApplication())
        ExoPlayerManager.setMediaItem(getApplication())
    }

    fun setPausedPositionToZero() {
        ExoPlayerManager.currentPlaybackPosition = 0
    }

    fun playPause() {
        if (ExoPlayerManager.isPlaying){
            ExoPlayerManager.pause()
        } else {
            ExoPlayerManager.play()
        }
    }

    fun updatePlayer1Volume(volume: Float) {
        player1Volume = volume
        ExoPlayerManager.setPlayerVolume(1, volume)
    }

    fun updatePlayer2Volume(volume: Float) {
        player2Volume = volume
        ExoPlayerManager.setPlayerVolume(2, volume)
    }

    fun updatePlayer3Volume(volume: Float) {
        player3Volume = volume
        ExoPlayerManager.setPlayerVolume(3, volume)
    }

    fun updatePlayer4Volume(volume: Float) {
        player4Volume = volume
        ExoPlayerManager.setPlayerVolume(4, volume)
    }

    fun updatePlayer5Volume(volume: Float) {
        player5Volume = volume
        ExoPlayerManager.setPlayerVolume(5, volume)
    }
}

class PlayerViewModelFactory(private val application: Application) : ViewModelProvider.Factory {
    @OptIn(UnstableApi::class)
    override fun <T : ViewModel> create(modelClass: Class<T>): T {
        if (modelClass.isAssignableFrom(PlayerViewModel::class.java)) {
            @Suppress("UNCHECKED_CAST")
            return PlayerViewModel(application) as T
        }
        throw IllegalArgumentException("Unknown ViewModel class")
    }
}