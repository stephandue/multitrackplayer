package com.stephanduechtel.fiveloop

import android.content.Context
import androidx.media3.common.MediaItem
import androidx.media3.exoplayer.ExoPlayer
import androidx.media3.exoplayer.source.DefaultMediaSourceFactory

object ExoPlayerManager {
    private var exoPlayerBass: ExoPlayer? = null
    private var exoPlayerDrums: ExoPlayer? = null
    private var exoPlayerOther: ExoPlayer? = null
    private var exoPlayerVocals: ExoPlayer? = null
    private var exoPlayerClick: ExoPlayer? = null

    var currentPlaybackPosition: Long = 0L

    var isPlaying: Boolean = false

    fun initialize(context: Context) {
        if (exoPlayerBass == null) {
            exoPlayerBass = ExoPlayer.Builder(context)
                .setMediaSourceFactory(DefaultMediaSourceFactory(context))
                .build()
        }
        if (exoPlayerDrums == null) {
            exoPlayerDrums = ExoPlayer.Builder(context)
                .setMediaSourceFactory(DefaultMediaSourceFactory(context))
                .build()
        }
        if (exoPlayerOther == null) {
            exoPlayerOther = ExoPlayer.Builder(context)
                .setMediaSourceFactory(DefaultMediaSourceFactory(context))
                .build()
        }
        if (exoPlayerVocals == null) {
            exoPlayerVocals = ExoPlayer.Builder(context)
                .setMediaSourceFactory(DefaultMediaSourceFactory(context))
                .build()
        }
        if (exoPlayerClick == null) {
            exoPlayerClick = ExoPlayer.Builder(context)
                .setMediaSourceFactory(DefaultMediaSourceFactory(context))
                .build()
        }
    }

    fun setMediaItem(context: Context) {
        val localURLBass = "android.resource://${context.packageName}/raw/bass"
        val localURLDrums = "android.resource://${context.packageName}/raw/drums"
        val localURLOther = "android.resource://${context.packageName}/raw/other"
        val localURLVocals = "android.resource://${context.packageName}/raw/vocals"
        val localURLClick = "android.resource://${context.packageName}/raw/click"

        // Create media items from the raw resource URIs
        val mediaItem = MediaItem.fromUri(localURLBass)
        exoPlayerBass?.setMediaItem(mediaItem)
        exoPlayerBass?.prepare()

        val mediaItem2 = MediaItem.fromUri(localURLDrums)
        exoPlayerDrums?.setMediaItem(mediaItem2)
        exoPlayerDrums?.prepare()

        val mediaItem3 = MediaItem.fromUri(localURLOther)
        exoPlayerOther?.setMediaItem(mediaItem3)
        exoPlayerOther?.prepare()

        val mediaItem4 = MediaItem.fromUri(localURLVocals)
        exoPlayerVocals?.setMediaItem(mediaItem4)
        exoPlayerVocals?.prepare()

        val mediaItem5 = MediaItem.fromUri(localURLClick)
        exoPlayerClick?.setMediaItem(mediaItem5)
        exoPlayerClick?.prepare()
    }

    fun play() {
        exoPlayerBass?.seekTo(currentPlaybackPosition)
        exoPlayerDrums?.seekTo(currentPlaybackPosition)
        exoPlayerOther?.seekTo(currentPlaybackPosition)
        exoPlayerVocals?.seekTo(currentPlaybackPosition)
        exoPlayerClick?.seekTo(currentPlaybackPosition)

        exoPlayerBass?.play()
        exoPlayerDrums?.play()
        exoPlayerOther?.play()
        exoPlayerVocals?.play()
        exoPlayerClick?.play()
        isPlaying = true
    }

    fun pause() {
        exoPlayerBass?.pause()
        exoPlayerDrums?.pause()
        exoPlayerOther?.pause()
        exoPlayerVocals?.pause()
        exoPlayerClick?.pause()
        currentPlaybackPosition = exoPlayerBass?.currentPosition ?: 0L
        isPlaying = false
    }

    fun setPlayerVolume(player: Int, volume: Float) {
        when (player) {
            1 -> exoPlayerBass?.volume = volume
            2 -> exoPlayerDrums?.volume = volume
            3 -> exoPlayerOther?.volume = volume
            4 -> exoPlayerVocals?.volume = volume
            5 -> exoPlayerClick?.volume = volume
        }
    }


    fun release() {
        exoPlayerBass?.release()
        exoPlayerBass = null
        exoPlayerDrums?.release()
        exoPlayerDrums = null
        exoPlayerOther?.release()
        exoPlayerOther = null
        exoPlayerVocals?.release()
        exoPlayerVocals = null
        exoPlayerClick?.release()
        exoPlayerClick = null
    }


}
