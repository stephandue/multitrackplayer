#
# Copyright 2019 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

cmake_minimum_required(VERSION 3.4.1)

# Pull in parselib
set (PARSELIB_DIR ../../../../parselib)
#message("PARSELIB_DIR = " + ${PARSELIB_DIR})

# Pull in iolib
set (IOLIB_DIR ../../../../iolib)
#message("IOLIB_DIR = " + ${IOLIB_DIR})

# Pull in soundtouch
set (SOUNDTOUCH_DIR ../../../../soundtouch)

# Pull in minimp3
set (MINIMP3_DIR ../../../../minimp3)

# Set the path to the Oboe library directory
set (OBOE_DIR ../../../../oboe)
#message("OBOE_DIR = " + ${OBOE_DIR})

add_subdirectory(${OBOE_DIR} ./oboe-bin)
add_subdirectory(${SOUNDTOUCH_DIR} ./soundtouch-bin)

# include folders
include_directories(
        ${OBOE_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}
        ${SOUNDTOUCH_DIR}/include
        ${PARSELIB_DIR}/src/main/cpp
        ${IOLIB_DIR}/src/main/cpp
        ${MINIMP3_DIR}
)

include(${PARSELIB_DIR}/src/main/cpp/CMakeLists.txt)
include(${IOLIB_DIR}/src/main/cpp/CMakeLists.txt)


# App specific sources
set (APP_SOURCES
        DrumPlayerJNI.cpp
        minimp3_wrapper.cpp
        )

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Ofast") #recommended here: https://medium.com/@donturner/debugging-audio-glitches-on-android-ed10782f9c64

# Build the drumthumper (native) library
add_library(drumthumper SHARED
        ${APP_SOURCES}
        )

# Enable optimization flags: if having problems with source level debugging,
# disable -Ofast ( and debug ), re-enable after done debugging.
#target_compile_options(drumthumper PRIVATE -Wall -Werror "$<$<CONFIG:RELEASE>:-Ofast>")
target_compile_options(drumthumper PRIVATE -Wall -Werror -Ofast)

target_link_libraries( # Specifies the target library.
        drumthumper

        -Wl,--whole-archive
        iolib
        parselib
        -Wl,--no-whole-archive

        oboe

        SoundTouch

        # Links the target library to the log library
        # included in the NDK.
        log)
