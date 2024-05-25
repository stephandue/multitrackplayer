package com.stephanduechtel.multitrackplayer

import android.app.Application
import android.content.Context
import android.content.res.AssetManager
import android.media.AudioManager
import android.media.audiofx.BassBoost
import android.media.audiofx.Equalizer
import android.util.Log
import androidx.annotation.OptIn
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.media3.common.util.UnstableApi
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import java.io.IOException


class PlayerViewModel(application: Application): AndroidViewModel(application) {

    val TAG: String = "DrumPlayer"

    var player1Volume by mutableStateOf(1f)
    var player2Volume by mutableStateOf(1f)
    var player3Volume by mutableStateOf(1f)
    var player4Volume by mutableStateOf(1f)
    var player5Volume by mutableStateOf(1f)

    private var currentTempo: Float = 1.0f
    private var currentPitch: Float = 0.0f

    val GAIN_FACTOR = 100.0f

    private var job: Job? = null
    private var audioSessionId: Int = 0
    private var equalizer: Equalizer? = null

    init {
        println("init PlayerViewModel")
        //ExoPlayerManager.initialize(getApplication())
        //ExoPlayerManager.setMediaItem(getApplication())
        System.loadLibrary("drumthumper")
        openAudioManager()
        setupAudioStreamNative(2, audioSessionId)
        //loadWavAssets(getApplication())
        loadWavAssets(application.assets)
        startAudioStreamNative()
        startSampleIndexLogging()
    }

    override fun onCleared() {
        super.onCleared()
        job?.cancel()
        releaseAudioEffects()
    }

    fun openAudioManager() {
        val audioManager = getApplication<Application>().getSystemService(Context.AUDIO_SERVICE) as AudioManager
        audioSessionId = audioManager.generateAudioSessionId()
        Log.d(TAG, "Generated Audio Session ID: $audioSessionId")
        applyAudioEffects()
    }

    private fun applyAudioEffects() {
        equalizer = Equalizer(0, audioSessionId).apply {
            enabled = false // set to true to enable the equalizer
            val bandCount = numberOfBands
            for (i in 0 until bandCount) {
                val lowerBandLevel = getBandLevelRange()[0]
                setBandLevel(i.toShort(), lowerBandLevel) // Set each band to its minimum value for a noticeable effect
            }
        }
        Log.d(TAG, "Equalizer effect applied with all bands set to minimum")
    }

    private fun releaseAudioEffects() {
        equalizer?.release()
    }

    private fun gainPosToGainVal(pos: Int) : Float {
        // map 0 -> 200 to 0.0f -> 2.0f
        return pos.toFloat() / GAIN_FACTOR
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
            // I only start one player here as the reference player, all the others will be started in SimpleMultiPlayer::triggerDown
//            stopTrigger(1)
//            stopTrigger(2)
//            stopTrigger(3)
        } else {
            trigger(0)
            // I only stop one player here as the reference player, all the others will be started in SimpleMultiPlayer::triggerUp
//            trigger(1)
//            trigger(2)
//            trigger(3)
        }

    }

    fun updatePlayer1Volume(volume: Float) {
        player1Volume = volume
        setGain(0, volume)
        //ExoPlayerManager.setPlayerVolume(1, volume)
    }

    fun updatePlayer2Volume(volume: Float) {
        player2Volume = volume
        setGain(1, volume)
        //ExoPlayerManager.setPlayerVolume(2, volume)
    }

    fun updatePlayer3Volume(volume: Float) {
        player3Volume = volume
        setGain(2, volume)
        //ExoPlayerManager.setPlayerVolume(3, volume)
    }

    fun updatePlayer4Volume(volume: Float) {
        player4Volume = volume
        setGain(3, volume)
        //ExoPlayerManager.setPlayerVolume(4, volume)
    }

    fun updatePlayer5Volume(volume: Float) {
        player5Volume = volume
        setGain(4, volume)
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

    // Function to decrease the tempo by 0.1
    fun tempoDown() {
        currentTempo -= 0.05f
        // Call native method to update tempo
        setTempoNative(currentTempo)
    }

    // Function to increase the tempo by 0.1
    fun tempoUp() {
        currentTempo += 0.05f
        // Call native method to update tempo
        setTempoNative(currentTempo)
    }

    // Function to decrease the pitch by one halftone (1.0)
    fun pitchDown() {
        currentPitch -= 1.0f
        // Call native method to update pitch
        setPitchSemiTonesNative(currentPitch)
    }

    // Function to increase the pitch by one halftone (1.0)
    fun pitchUp() {
        currentPitch += 1.0f
        // Call native method to update pitch
        setPitchSemiTonesNative(currentPitch)
    }

    fun isSamplePlaying(index: Int): Boolean {
        return isSampleSourcePlaying(index)
    }

    private external fun setupAudioStreamNative(numChannels: Int, audioSessionId: Int)
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
    external fun setTempoNative(tempo: Float)
    external fun setPitchSemiTonesNative(pitch: Float)
    external fun getTempoNative(): Float
    external fun getPitchSemiTonesNative(): Float

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