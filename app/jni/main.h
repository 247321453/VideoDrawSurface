//
// Created by user on 2019/4/2.
//

#ifndef FFMPEGNATIVEWINDOW_MAIN_H
#define FFMPEGNATIVEWINDOW_MAIN_H
#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "debug.h"
#include "video_player.h"

#define JNI_CLASS_NAME "net/kk/ffmpeg/VideoPlayer"
JNIEnv *getEnv();
JNIEnv *ensureEnvCreated();

#endif //FFMPEGNATIVEWINDOW_MAIN_H
