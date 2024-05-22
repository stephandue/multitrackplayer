/*
 * Copyright (C) 2019 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string.h>
#include <vector>

#include "wav/WavStreamReader.h"

#include "OneShotSampleSource.h"

namespace iolib {

void OneShotSampleSource::mixAudio(float* outBuff, int numChannels, int32_t numFrames) {
    int32_t numSamples = mSampleBuffer->getNumSamples();
    int32_t sampleChannels = mSampleBuffer->getProperties().channelCount;
    int32_t samplesLeft = numSamples - mCurSampleIndex;
    int32_t numWriteFrames = mIsPlaying
                         ? std::min(numFrames, samplesLeft / sampleChannels)
                         : 0;
    //LOGD("numSamples: %d, sampleChannels: %d, samplesLeft: %d, numWriteFrames: %d", numSamples, sampleChannels, samplesLeft, numWriteFrames);

    if (numWriteFrames != 0) {
        const float* data  = mSampleBuffer->getSampleData();

        // Buffer to hold processed samples
        std::vector<float> processedSamples(numWriteFrames * sampleChannels);

        // Feed the required number of samples to SoundTouch
        mSoundTouch.putSamples(data + mCurSampleIndex, numWriteFrames);

        // Calculate the actual number of processed frames
        //mSoundTouch.receiveSamples(processedSamples.data(), numWriteFrames);
        int numProcessedFrames = mSoundTouch.receiveSamples(processedSamples.data(), numWriteFrames);

        //LOGD("numProcessedFrames: %d", numProcessedFrames);

        // Adjust numWriteFrames based on actual processed frames
        numWriteFrames = numProcessedFrames;

        // Ensure we have enough samples processed
        if (numProcessedFrames < numWriteFrames) {
            // Handle the case where fewer samples are received
            memset(processedSamples.data() + numProcessedFrames * sampleChannels, 0,
                   (numWriteFrames - numProcessedFrames) * sampleChannels * sizeof(float));
        }

        // Ensure we have enough samples processed
        /*int numReceived = mSoundTouch.receiveSamples(processedSamples.data(), numWriteFrames);
        if (numReceived < numWriteFrames) {
            // Handle the case where fewer samples are received
            memset(processedSamples.data() + numReceived * sampleChannels, 0,
                   (numWriteFrames - numReceived) * sampleChannels * sizeof(float));
        }*/

        /*int numReceived = 0;
        int totalProcessed = 0;

        while (totalProcessed < numWriteFrames) {
            numReceived = mSoundTouch.receiveSamples(processedSamples.data() + totalProcessed * sampleChannels, numWriteFrames - totalProcessed);
            if (numReceived == 0) {
                // If no samples received, break the loop to avoid infinite loop
                break;
            }
            totalProcessed += numReceived;
        }*/

        if ((sampleChannels == 1) && (numChannels == 1)) {
            // MONO output from MONO samples
            for (int32_t frameIndex = 0; frameIndex < numWriteFrames; frameIndex++) {
                outBuff[frameIndex] += processedSamples[frameIndex] * mGain;
            }
        } else if ((sampleChannels == 1) && (numChannels == 2)) {
            // STEREO output from MONO samples
            int dstSampleIndex = 0;
            for (int32_t frameIndex = 0; frameIndex < numWriteFrames; frameIndex++) {
                outBuff[dstSampleIndex++] += processedSamples[frameIndex] * mLeftGain;
                outBuff[dstSampleIndex++] += processedSamples[frameIndex] * mRightGain;
            }
        } else if ((sampleChannels == 2) && (numChannels == 1)) {
            // MONO output from STEREO samples
            int dstSampleIndex = 0;
            for (int32_t frameIndex = 0; frameIndex < numWriteFrames; frameIndex++) {
                outBuff[dstSampleIndex++] += processedSamples[frameIndex * 2] * mLeftGain +
                                             processedSamples[frameIndex * 2 + 1] * mRightGain;
            }
        } else if ((sampleChannels == 2) && (numChannels == 2)) {
            // STEREO output from STEREO samples
            int dstSampleIndex = 0;
            for (int32_t frameIndex = 0; frameIndex < numWriteFrames; frameIndex++) {
                outBuff[dstSampleIndex++] += processedSamples[frameIndex * 2] * mLeftGain;
                outBuff[dstSampleIndex++] += processedSamples[frameIndex * 2 + 1] * mRightGain;
            }
        }

        mCurSampleIndex += numWriteFrames * sampleChannels;
        if (mCurSampleIndex >= numSamples) {
            mIsPlaying = false;
        }

        // Without any processing
        /*if ((sampleChannels == 1) && (numChannels == 1)) {
            // MONO output from MONO samples
            for (int32_t frameIndex = 0; frameIndex < numWriteFrames; frameIndex++) {
                outBuff[frameIndex] += data[mCurSampleIndex++] * mGain;
            }
        } else if ((sampleChannels == 1) && (numChannels == 2)) {
            // STEREO output from MONO samples
            int dstSampleIndex = 0;
            for (int32_t frameIndex = 0; frameIndex < numWriteFrames; frameIndex++) {
                outBuff[dstSampleIndex++] += data[mCurSampleIndex] * mLeftGain;
                outBuff[dstSampleIndex++] += data[mCurSampleIndex++] * mRightGain;
            }
        } else if ((sampleChannels == 2) && (numChannels == 1)) {
            // MONO output from STEREO samples
            int dstSampleIndex = 0;
            for (int32_t frameIndex = 0; frameIndex < numWriteFrames; frameIndex++) {
                outBuff[dstSampleIndex++] += data[mCurSampleIndex++] * mLeftGain +
                                             data[mCurSampleIndex++] * mRightGain;
            }
        } else if ((sampleChannels == 2) && (numChannels == 2)) {
            // STEREO output from STEREO samples
            int dstSampleIndex = 0;
            for (int32_t frameIndex = 0; frameIndex < numWriteFrames; frameIndex++) {
                outBuff[dstSampleIndex++] += data[mCurSampleIndex++] * mLeftGain;
                outBuff[dstSampleIndex++] += data[mCurSampleIndex++] * mRightGain;
            }
        }

        if (mCurSampleIndex >= numSamples) {
            mIsPlaying = false;
        }*/

    }  else {
        LOGD("No frames to write.");
    }

    // silence
    // no need as the output buffer would need to have been filled with silence
    // to be mixed into
}

} // namespace wavlib
