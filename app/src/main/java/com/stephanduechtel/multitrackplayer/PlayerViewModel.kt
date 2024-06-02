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
import kotlinx.coroutines.withContext
import java.io.File
import okhttp3.OkHttpClient
import okhttp3.Request
import kotlinx.coroutines.async
import kotlinx.coroutines.awaitAll


class PlayerViewModel(application: Application): AndroidViewModel(application) {

    val TAG: String = "DrumPlayer"

    data class Mp3File(val filePath: String, val index: Int)

    var player1Volume by mutableStateOf(1f)
    var player2Volume by mutableStateOf(1f)
    var player3Volume by mutableStateOf(1f)
    var player4Volume by mutableStateOf(1f)
    var player5Volume by mutableStateOf(1f)

    private var currentTempo: Float = 1.0f
    private var currentPitch: Float = 0.0f

    val GAIN_FACTOR = 100.0f

    private var job: Job? = null

    var currentTimeInSeconds by mutableStateOf(0f)
        private set

    var totalLengthInSeconds by mutableStateOf(0f)
        private set

    var isDragging by mutableStateOf(false)
        private set

    var seekToTime by mutableStateOf(0f)

    // State variable
    var loopState: LoopState = LoopState.Inactive
        private set

    // Time variables
    var loopIn: Float = 0f
        private set

    var loopOut: Float = 0f
        private set

    init {
        println("+++ init PlayerViewModel")
        System.loadLibrary("drumthumper")
        checkIfDirectoryExists()
    }

    override fun onCleared() {
        super.onCleared()
        println("+++ onCleared")
        job?.cancel()
        unloadWavAssetsNative()
        teardownAudioStreamNative()
    }

    fun initAudioPlayers() {
        setupAudioStreamNative(2)
        loadMp3Assets()

    }

    fun checkIfDirectoryExists() {
        val fileManager = File(getApplication<Application>().filesDir, "documents")
        val documentsDirectory = File(fileManager, "song")
        val directoryPath = documentsDirectory.path

        //return

        if (documentsDirectory.exists() && documentsDirectory.isDirectory) {
            try {
                val files = documentsDirectory.listFiles()?.map { it.name } ?: emptyList()
                val desiredFiles = listOf("bass.mp3", "vocals.mp3", "drums.mp3", "other.mp3")
                val containsAllDesiredFiles = desiredFiles.all { it in files }
                val clickFiles = listOf("click.mp3", "clickonset.mp3")
                val containsAllClickFiles = clickFiles.all { it in files }

                if (containsAllDesiredFiles && containsAllClickFiles) {
                    // All desired files are present in the folder
                    initAudioPlayers()
                } else if (containsAllDesiredFiles && !containsAllClickFiles) {
                    initAudioPlayers()
                    println("+++ containsAllDesiredFiles")
                    //downloadClickTracksAfterStemsAreStored()
                } else {
                    // Not all desired files are present in the folder
                    downloadFiles { success ->
                        if (success) {
                            println("All files downloaded successfully.")
                            initAudioPlayers()
                        } else {
                            println("Failed to download files.")
                        }
                    }
                }
            } catch (e: Exception) {
                println("Error listing files in folder: ${e.message}")
            }
        } else {
            try {
                documentsDirectory.mkdirs()
                // Directory created
                downloadFiles { success ->
                    if (success) {
                        println("All files downloaded successfully.")
                        initAudioPlayers()
                    } else {
                        println("Failed to download files.")
                    }
                }
            } catch (e: Exception) {
                println("Error creating directory: ${e.message}")
            }
        }
    }

    fun downloadFiles(completionHandler: (Boolean) -> Unit) {
        val documentsDirectory = File(getApplication<Application>().filesDir, "documents")
        val localURLBass = File(documentsDirectory, "song/bass.mp3")
        val localURLDrums = File(documentsDirectory, "song/drums.mp3")
        val localURLOther = File(documentsDirectory, "song/other.mp3")
        val localURLVocals = File(documentsDirectory, "song/vocals.mp3")
        val localURLClick = File(documentsDirectory, "song/click.mp3")

        val urls = mapOf(
            "https://www.fiveloop.io/bass.mp3" to localURLBass,
            "https://www.fiveloop.io/drums.mp3" to localURLDrums,
            "https://www.fiveloop.io/other.mp3" to localURLOther,
            "https://www.fiveloop.io/vocals.mp3" to localURLVocals,
            "https://www.fiveloop.io/click.mp3" to localURLClick
        )

        // Create the documents directory if it does not exist
        if (!documentsDirectory.exists()) {
            documentsDirectory.mkdirs()
        }

        val client = OkHttpClient()

        CoroutineScope(Dispatchers.IO).launch {
            var allDownloadsSuccessful = true

            for ((url, file) in urls) {
                val request = Request.Builder().url(url).build()

                try {
                    client.newCall(request).execute().use { response ->
                        if (!response.isSuccessful) throw IOException("Failed to download file: $url")

                        response.body?.byteStream()?.let { inputStream ->
                            file.outputStream().use { outputStream ->
                                inputStream.copyTo(outputStream)
                            }
                        }
                    }
                } catch (e: Exception) {
                    e.printStackTrace()
                    allDownloadsSuccessful = false
                    break
                }
            }

            withContext(Dispatchers.Main) {
                completionHandler(allDownloadsSuccessful)
            }
        }
    }

    private fun gainPosToGainVal(pos: Int) : Float {
        // map 0 -> 200 to 0.0f -> 2.0f
        return pos.toFloat() / GAIN_FACTOR
    }

    private fun startSampleIndexLogging() {
        job = CoroutineScope(Dispatchers.IO).launch {
            while (isActive) {
                if (isSampleSourcePlaying(0)) {
                    val timeInSeconds = getCurrentTimeInSeconds(0)
                    withContext(Dispatchers.Main) {
                        currentTimeInSeconds = timeInSeconds
                        if (loopState == LoopState.Active && currentTimeInSeconds >= loopOut) {
                            setPlaybackTimeInSeconds(loopIn)
                        }
                    }
                }
                delay(50)
            }
        }
    }

    fun loop() {
        when (loopState) {
            is LoopState.Inactive -> {
                loopState = LoopState.Pending
                loopIn = getCurrentTimeInSeconds(0)
            }
            is LoopState.Pending -> {
                loopState = LoopState.Active
                loopOut = getCurrentTimeInSeconds(0)
            }
            is LoopState.Active -> {
                loopState = LoopState.Inactive
            }
        }
        // Print statements for debugging
        println("Loop state changed to: $loopState")
        println("LoopIn time: $loopIn")
        println("LoopOut time: $loopOut")
    }

    fun seekTo(newTime: Float) {
        seekToTime = newTime
    }

    fun onSeekStart() {
        println("onSeekStart")
        isDragging = true
    }

    fun onSeekEnd() {
        println("onSeekEnd $seekToTime")
        isDragging = false
        setPlaybackTimeInSeconds(seekToTime)
    }


    fun playPause() {
        if (isSamplePlaying(0)) {
            stopTrigger(0)
            // I only start one player here as the reference player, all the others will be started in SimpleMultiPlayer::triggerDown
        } else {
            trigger(0)
            // I only stop one player here as the reference player, all the others will be started in SimpleMultiPlayer::triggerUp
        }

    }

    fun updatePlayer1Volume(volume: Float) {
        player1Volume = volume
        setGain(0, volume)
    }

    fun updatePlayer2Volume(volume: Float) {
        player2Volume = volume
        setGain(1, volume)
    }

    fun updatePlayer3Volume(volume: Float) {
        player3Volume = volume
        setGain(2, volume)
    }

    fun updatePlayer4Volume(volume: Float) {
        player4Volume = volume
        setGain(3, volume)
    }

    fun updatePlayer5Volume(volume: Float) {
        player5Volume = volume
        setGain(4, volume)
    }

    fun loadMp3Assets() {
        val documentsDirectory = File(getApplication<Application>().filesDir, "documents")
        val localURLBass = File(documentsDirectory, "song/bass.mp3").absolutePath
        val localURLDrums = File(documentsDirectory, "song/drums.mp3").absolutePath
        val localURLOther = File(documentsDirectory, "song/other.mp3").absolutePath
        val localURLVocals = File(documentsDirectory, "song/vocals.mp3").absolutePath
        val localURLClick = File(documentsDirectory, "song/click.mp3").absolutePath

        val mp3Files = listOf(
            Mp3File(localURLBass, 0),
            Mp3File(localURLDrums, 1),
            Mp3File(localURLOther, 2),
            Mp3File(localURLVocals, 3),
            Mp3File(localURLClick, 4)
        )

        loadMultipleMp3Assets(mp3Files, 0.0f) { success ->
            if (success) {
                println("All MP3 files loaded successfully.")
                val indexesToIgnore = intArrayOf(1, 4, 5) // 1 decides if drums should be pitched or not
                setIgnorePitchIndexesNative(indexesToIgnore)
                startAudioStreamNative()
                totalLengthInSeconds = getTotalLengthInSeconds(0)
                startSampleIndexLogging()
            } else {
                println("Failed to load one or more MP3 files.")
            }
        }
    }

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

    fun loadMultipleMp3Assets(mp3Files: List<Mp3File>, pan: Float, completionHandler: (Boolean) -> Unit) {
        CoroutineScope(Dispatchers.IO).launch {
            var allSuccessful = true

            for (mp3File in mp3Files) {
                val success = try {
                    loadMp3AssetNative(mp3File.filePath, mp3File.index, pan)
                    true
                } catch (e: Exception) {
                    e.printStackTrace()
                    false
                }

                if (!success) {
                    allSuccessful = false
                    break
                }
            }

            withContext(Dispatchers.Main) {
                completionHandler(allSuccessful)
            }
        }
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

    external fun setPlaybackTimeInSeconds(newTime: Float)

    external fun getTotalLengthInSeconds(index: Int): Float

    external fun getOutputReset() : Boolean
    external fun clearOutputReset()

    external fun restartStream()

    external fun isSampleSourcePlaying(index: Int): Boolean
    external fun setTempoNative(tempo: Float)
    external fun setPitchSemiTonesNative(pitch: Float)
    external fun getTempoNative(): Float
    external fun getPitchSemiTonesNative(): Float
    external fun loadMp3AssetNative(filePath: String, index: Int, pan: Float)
    external fun setIgnorePitchIndexesNative(indexes: IntArray)

}

sealed class LoopState {
    object Inactive : LoopState()
    object Pending : LoopState()
    object Active : LoopState()
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