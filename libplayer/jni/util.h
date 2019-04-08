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

int av_frame_to_nv21(AVFrame *frame, uint8_t *nv21);
int av_frame_to_i420(AVFrame *frame, uint8_t *i420);
int av_frame_to_jpeg(AVFrame *frame, uint8_t *jpeg, int q);
int av_get_rotation(AVStream *st);
long getCurTime();
int av_yuv420p_to_nv21(AVFrame *frame, int width, int height, uint8_t *nv21Data);
int av_frame_rotate(AVFrame *src, int rotation, AVFrame *dst);
int av_frame_rotate_crop(AVFrame *src, int rotation,
                         int left, int top, int dst_width, int dst_height, AVFrame *dst);
int av_frame_scale(AVFrame *src, AVFrame *dst, int dst_width, int dst_height);
#endif //FFMPEGNATIVEWINDOW_AV_ROTATION_H
