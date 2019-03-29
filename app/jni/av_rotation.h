//
// Created by user on 2019/4/2.
//

#ifndef FFMPEGNATIVEWINDOW_AV_ROTATION_H
#define FFMPEGNATIVEWINDOW_AV_ROTATION_H
extern "C"{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
};

#define ROTATION_0 0
#define ROTATION_90 1
#define ROTATION_180 2
#define ROTATION_270 3

int av_get_rotation(AVStream *st);
void av_frame_rotate_90(AVFrame *src, AVFrame *des);
void av_frame_rotate_180(AVFrame *src, AVFrame *des);
void av_frame_rotate_270(AVFrame *src, AVFrame *des);

#endif //FFMPEGNATIVEWINDOW_AV_ROTATION_H
