#3Damnesic 

A Work In Progress media player for 3ds using ffmpeg !

#Requirements

* DevkitArm
* Latest ctrulib
* ffmpeg compiled with the following instructions

#Building 

##FFMPEG

* Copy the ffmpeg-configure3ds script in your ffmpeg source folder
* Open a shell/command line in ffmpeg directory
    - Windows users please use `sh` before starting the script
* ./ffmpeg-configure3ds
* make install

This will compile ffmpeg (with only a few features) with devkitArm and install it as a portlib

##3Damnesic

###With the Makefile

Simply use `make`.

###With CMake

* `mkdir cbuild && cd cbuild`
    * On *NIX `cmake -DCMAKE_TOOLCHAIN_FILE=DevkitArm3DS.cmake ..`
    * On Windows `cmake -DCMAKE_TOOLCHAIN_FILE=DevkitArm3DS.cmake -G"Unix Makefiles" ..`
* `make`

More information on the [3ds-cmake](https://github.com/Lectem/3ds-cmake) repository.

#Usage

At the moment, you have to specify the file path in the main.c file at compilation time.


# Features

* Video
    - MPEG4, H.264
    - Hardware acceleration with Y2R for YUV -> RGB conversions
    
# TODO

* Audio
* Sync and time adjustment
* File Browser and Menu
* Subtitles
* More formats and track selection
* Use the MVD service for the new3ds

#Random informations

Use a video with dimensions multiple of 8 for best performance !

Videos up to 1024x1024 are supported (but eh, that won't run fullspeed you know)
- Actually only if width < 800 if you set up the framebuffers to be using RGBA

Prefer simple MPEG4 to H.264 ! (H.264 is  ~4 times slower)
 
Some stats (video only, old 3ds) :

* 400x240 mpeg4 -> 37fps
* 400x240 h264  -> 16fps

The new3ds is way faster