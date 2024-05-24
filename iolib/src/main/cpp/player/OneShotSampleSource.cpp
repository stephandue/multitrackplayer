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
    LOGD("numSamples: %d, sampleChannels: %d, samplesLeft: %d, numWriteFrames: %d deadline: %f ms", numSamples, sampleChannels, samplesLeft, numWriteFrames, numWriteFrames / 44.1);

    if (numWriteFrames != 0) {
        const float* data  = mSampleBuffer->getSampleData();

//        // Buffer to hold processed samples
//        std::vector<float> processedSamples(numWriteFrames * sampleChannels);
//
//        // Feed the required number of samples to SoundTouch
//        mSoundTouch.putSamples(data + mCurSampleIndex, numWriteFrames);
//
//        // Calculate the actual number of processed frames
//        mSoundTouch.receiveSamples(processedSamples.data(), numWriteFrames);



//        mCurSampleIndex2 = mCurSampleIndex;


        int dstSampleIndex = 0;
        for (int32_t frameIndex = 0; frameIndex < numWriteFrames; frameIndex++) {
//            outBuff[dstSampleIndex++] += processedSamples[frameIndex * 2] * mLeftGain;
//            outBuff[dstSampleIndex++] += processedSamples[frameIndex * 2 + 1] * mRightGain;
            outBuff[dstSampleIndex++] += data[mCurSampleIndex++] * mLeftGain;
            outBuff[dstSampleIndex++] += data[mCurSampleIndex++] * mRightGain;
//            mCurSampleIndex2++;
//            mCurSampleIndex2++;
        }
//        mCurSampleIndex += numWriteFrames * sampleChannels;

        if (mCurSampleIndex >= numSamples) {
            mIsPlaying = false;
        }

    }  else {
        LOGD("No frames to write.");
    }

    // silence
    // no need as the output buffer would need to have been filled with silence
    // to be mixed into
}

} // namespace wavlib
