package com.stephanduechtel.multitrackplayer

import android.app.Application
import android.content.Context
import android.content.res.AssetManager
import android.util.Log
import androidx.annotation.OptIn
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.media3.common.util.UnstableApi
import com.stephanduechtel.fiveloop.ExoPlayerManager
import kotlinx.coroutines.*
import java.io.IOException

class PlayerViewModel(application: Application): AndroidViewModel(application) {

    val TAG: String = "DrumPlayer"

    var player1Volume by mutableStateOf(1f)
    var player2Volume by mutableStateOf(1f)
    var player3Volume by mutableStateOf(1f)
    var player4Volume by mutableStateOf(1f)
    var player5Volume by mutableStateOf(1f)

    private var job: Job? = null

    init {
        println("init PlayerViewModel")
        //ExoPlayerManager.initialize(getApplication())
        //ExoPlayerManager.setMediaItem(getApplication())
        System.loadLibrary("drumthumper")
        setupAudioStreamNative(2)
        //loadWavAssets(getApplication())
        loadWavAssets(application.assets)
        startAudioStreamNative()
        startSampleIndexLogging()
    }

    override fun onCleared() {
        super.onCleared()
        job?.cancel()
    }

    private fun startSampleIndexLogging() {
        job = CoroutineScope(Dispatchers.IO).launch {
            while (isActive) {
                if (isSampleSourcePlaying(0)) {
                    val timeInSconds = getCurrentTimeInSeconds(0)
//                    Log.d(TAG, "Current Time In Seconds: $timeInSconds")
                }
                delay(50)
            }
        }
    }

    fun setPausedPositionToZero() {
        //ExoPlayerManager.currentPlaybackPosition = 0
    }

    fun playPause() {
        /*if (ExoPlayerManager.isPlaying){
            ExoPlayerManager.pause()
        } else {
            ExoPlayerManager.play()
        }*/
        if (isSamplePlaying(0)) {
            stopTrigger(0)
            stopTrigger(1)
            stopTrigger(2)
            stopTrigger(3)
        } else {
            trigger(0)
            trigger(1)
            trigger(2)
            trigger(3)
        }

    }

    fun updatePlayer1Volume(volume: Float) {
        player1Volume = volume
        //ExoPlayerManager.setPlayerVolume(1, volume)
    }

    fun updatePlayer2Volume(volume: Float) {
        player2Volume = volume
        //ExoPlayerManager.setPlayerVolume(2, volume)
    }

    fun updatePlayer3Volume(volume: Float) {
        player3Volume = volume
        //ExoPlayerManager.setPlayerVolume(3, volume)
    }

    fun updatePlayer4Volume(volume: Float) {
        player4Volume = volume
        //ExoPlayerManager.setPlayerVolume(4, volume)
    }

    fun updatePlayer5Volume(volume: Float) {
        player5Volume = volume
        //ExoPlayerManager.setPlayerVolume(5, volume)

    }

    fun loadWavAssets(assetMgr: AssetManager) {

        loadWavAsset(assetMgr, "1.wav", 0, 0f)
        loadWavAsset(assetMgr, "2.wav", 1, 0f)
        loadWavAsset(assetMgr, "3.wav", 2, 0f)
        loadWavAsset(assetMgr, "4.wav", 3, 0f)

        //loadWavAsset(assetMgr, "bass.wav", 0, 0f)
        //loadWavAsset(assetMgr, "drums.wav", 1, 0f)
        //loadWavAsset(assetMgr, "other.wav", 2, 0f)
        //loadWavAsset(assetMgr, "vocals.wav", 3, 0f)

    }

    /*fun loadWavAssets(context: Context) {
        loadWavAsset(context, R.raw.bass, 0, 0f)
        loadWavAsset(context, R.raw.drums, 1, 0f)
        loadWavAsset(context, R.raw.other, 2, 0f)
        loadWavAsset(context, R.raw.vocals, 3, 0f)
        loadWavAsset(context, R.raw.click, 4, 0f)

    }*/

    fun unloadWavAssets() {
        unloadWavAssetsNative()
    }

    private fun loadWavAsset(assetMgr: AssetManager, assetName: String, index: Int, pan: Float) {
        try {
            val assetFD = assetMgr.openFd(assetName)
            val dataStream = assetFD.createInputStream()
            val dataLen = assetFD.getLength().toInt()
            val dataBytes = ByteArray(dataLen)
            dataStream.read(dataBytes, 0, dataLen)
            loadWavAssetNative(dataBytes, index, pan)
            assetFD.close()
        } catch (ex: IOException) {
            Log.i(TAG, "IOException$ex")
        }
    }

    /*private fun loadWavAsset(context: Context, resId: Int, index: Int, pan: Float) {
        try {
            Log.i(TAG, "Loading WAV asset with resource ID: $resId")
            val inputStream = context.resources.openRawResource(resId)
            val dataBytes = inputStream.readBytes()
            Log.i(TAG, "Loaded ${dataBytes.size} bytes for WAV asset")
            loadWavAssetNative(dataBytes, index, pan)
            inputStream.close()
        } catch (ex: IOException) {
            Log.i(TAG, "IOException$ex")
        } catch (ex: Exception) {
            Log.e(TAG, "Unexpected exception while loading WAV asset: $ex")
        }
    }*/

    fun isSamplePlaying(index: Int): Boolean {
        return isSampleSourcePlaying(index)
    }

    private external fun setupAudioStreamNative(numChannels: Int)
    private external fun startAudioStreamNative()
    private external fun teardownAudioStreamNative()

    private external fun loadWavAssetNative(wavBytes: ByteArray, index: Int, pan: Float)
    private external fun unloadWavAssetsNative()

    external fun trigger(drumIndex: Int)
    external fun stopTrigger(drumIndex: Int)

    external fun setPan(index: Int, pan: Float)
    external fun getPan(index: Int): Float

    external fun setGain(index: Int, gain: Float)
    external fun getGain(index: Int): Float

    external fun getCurrentSampleIndex(index: Int): Int
    external fun getCurrentTimeInSeconds(index: Int): Float

    external fun getOutputReset() : Boolean
    external fun clearOutputReset()

    external fun restartStream()

    external fun isSampleSourcePlaying(index: Int): Boolean

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