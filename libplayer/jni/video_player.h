//
// Created by user on 2019/4/2.
//

#ifndef FFMPEGNATIVEWINDOW_FFMPEG_H
#define FFMPEGNATIVEWINDOW_FFMPEG_H

#include <jni.h>
#include <android/native_window.h>
#include "util.h"
#include "debug.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
};

namespace kk {

    struct VideoInfo {
        //视频原始尺寸
        int video_width;
        int video_height;
        int video_rotation = ROTATION_0;
        //当前角度
        int display_rotation;
        //当前播放尺寸
        int display_width;
        int display_height;
        bool need_crop;
        bool need_scale;

        //原始尺寸的坐标
        int crop_x;
        int crop_y;
        int crop_width;
        int crop_height;

        int rotate_width;
        int rotate_height;
        //
        int scale_width;
        int scale_height;
        bool need_swap_size;
    };

    static void
    initVideoSize(VideoInfo *size, int dst_width, int dst_height, int dst_rotation, bool stretch) {
        ALOGD("video:rotation=%d, width=%d,height=%d", size->video_rotation, size->video_width,
              size->video_height);
        //按照比例自动旋转
        int rotation = size->video_rotation;
        if (dst_rotation > 0) {
            rotation = (rotation + dst_rotation) % 4; // 0-3
            ALOGD("has pre rotate: pre=%d, now=%d", dst_rotation, rotation);
        }
        size->need_swap_size = (rotation == ROTATION_90 || rotation == ROTATION_270);
        if (size->need_swap_size) {
            size->rotate_width = size->video_height;
            size->rotate_height = size->video_width;
            ALOGD("swap size: pre=%dx%d, now=%dx%d",
                  size->video_width, size->video_height,
                  size->rotate_width, size->rotate_height);
        } else {
            size->rotate_width = size->video_width;
            size->rotate_height = size->video_height;
        }
        size->display_rotation = rotation;
        //旋转之后的
        size->display_width = size->rotate_width;
        size->display_height = size->rotate_height;
        size->crop_x = 0;
        size->crop_y = 0;
        size->crop_width = size->display_width;
        size->crop_height = size->display_height;
        size->scale_width = dst_width;
        size->scale_height = dst_height;

        int src_width = size->display_width;
        int src_height = size->display_height;
        if (dst_width > 0 && dst_height > 0) {
            //基于旋转后的宽高去计算缩放，裁剪
            if (dst_width == src_width && dst_height == src_height) {
                size->need_scale = false;
                size->need_crop = false;
                //宽高一样，不需要裁剪，不需要缩放
                ALOGD("size is same, don't scale and crop");
            } else {
                //默认
                size->need_crop = true;
                size->need_scale = true;
                if (!stretch) {
                    if ((((float) dst_width / (float) dst_height) ==
                         ((float) src_width / (float) src_height))) {
                        //一样的比例，不需要裁剪，直接缩放
                        ALOGD("don't crop");
                    } else {
                        //目标的宽和原始比例的宽一样
                        int w1 = src_width;//540/360*640=960
                        int h1 = (int) round(
                                (float) w1 / (float) dst_width * (float) dst_height);
                        //目标的高和原始比例的高一样
                        int h2 = src_height;//960/640*360=540
                        int w2 = (int) round(h2 / (float) dst_height * (float) dst_width);

                        if (h1 < src_height) {
                            //裁剪的高度小于原始高度，进行裁剪
                            size->crop_x = 0;
                            size->crop_y =
                                    (src_height - h1) / 4 * 2;//不能单数 26/2=13  26/4*2=12
                            size->crop_width = w1;
                            size->crop_height = h1;
                            ALOGD("need crop %d,%d,%d,%d", size->crop_x, size->crop_y,
                                  size->crop_width,
                                  size->crop_height);
                        } else if (w2 < src_width) {
                            //裁剪的宽度小于原始宽度，进行裁剪
                            //宽度比目标宽
                            size->crop_x = (src_width - w2) / 4 * 2;
                            size->crop_y = 0;
                            size->crop_width = w2;
                            size->crop_height = h2;
                            ALOGD("need crop %d,%d,%d,%d", size->crop_x, size->crop_y,
                                  size->crop_width,
                                  size->crop_height);
                        } else {
                            //
                            ALOGW("how to crop?");
                        }
                    }
                }
            }
        } else {
            size->need_scale = false;
            size->need_crop = false;
            ALOGD("don't scale and crop");
        }
//        if (need_swap_size) {
//            //上门是基于旋转后计算的，需要交换
//            int tmp = size->crop_x;
//            size->crop_x = size->crop_y;
//            size->crop_y = tmp;
//            tmp = size->crop_width;
//            size->crop_width = size->crop_height;
//            size->crop_height = tmp;
//            ALOGD("swap crop %d,%d %dx%d", size->crop_x, size->crop_y, size->crop_width,
//                  size->crop_height);
//        }
        if (size->need_scale) {
            size->display_width = size->scale_width;
            size->display_height = size->scale_height;
        }
    }

    class VideoPlayer {
    public:
        VideoPlayer();

        void SetSize(int width, int height, bool stretch, int preRotation) {
            mStretchMode = stretch;
            mPreviewWidth = width;
            mPreviewHeight = height;
            mPreRotation = preRotation;
            initVideoSize(&Info, width, height, preRotation, stretch);
        }

        void SetSurface(ANativeWindow *surface) {
            if (pNativeWindow != nullptr) {
                ANativeWindow_release(pNativeWindow);
                pNativeWindow = nullptr;
            }
            pNativeWindow = surface;
        }

        void SetDataSource(const char *path) {
            Release(false);
            ResetInfo(&Info);
            mVideoStream = -1;
            mPreLoad = false;
            mFileName = path;
        }

        void SetCallBack(jmethodID yuv_callback, jmethodID jpeg_callback, bool needNv21Data) {
            mYuvCallBackId = yuv_callback;
            mJpegCallBackId = jpeg_callback;
            mNeedNv21Data = needNv21Data;
        }

        int PreLoad();

        int Play(JNIEnv *env, jobject obj);

        void Stop();

        void Close();

        void Release(bool resize);

        int Seek(double time);

        double GetPlayDuration() {
            return mVideoCurDuration;
        }

        double GetVideoDuration() {
            return mVideoAllDuration;
        }

        uint8_t *GetVideoLastFrame() {
            return pTakeYuvBuf;
        }

        int GetVideoLastLength() {
            return pVideoYuvLen;
        }

        VideoInfo GetVideoInfo() {
            return Info;
        }

        bool IsPlaying() {
            return mPlaying;
        }

        int TakeImage(JNIEnv *env, jobject obj, jint width, jint height, jint rotation);

        ~VideoPlayer() {
            Close();
        }

    private:
        bool mPreLoad;
        //循环播放
        bool mPlaying;

        int mVideoStream = -1;
        // 用户需要的尺寸
        //拉伸
        bool mStretchMode = false;
        int mPreviewWidth = 0;
        int mPreviewHeight = 0;
        int mPreRotation = 0;

        VideoInfo Info;

        bool mNeedNv21Data = false;
        ANativeWindow *pNativeWindow = nullptr;
        AVFormatContext *pFormatCtx = nullptr;
        AVCodecContext *pCodecCtx = nullptr;
        AVCodec *pCodec = nullptr;
        AVFrame *pFrameRGBA = nullptr;
        AVFrame *pFrameNv21 = nullptr;
        AVFrame *pFrame = nullptr;
        AVFrame *pRotateCropFrame = nullptr;
        AVFrame *pScaleFrame = nullptr;
        AVRational pTimeBase;
        uint8_t *pScaleBuf = nullptr;
        uint8_t *pRotateCropBuf = nullptr;
        uint8_t *pRgbaBuf = nullptr;
        uint8_t *pNv21Buf = nullptr;
        //原始i420数据
        uint8_t *pTakeYuvBuf = nullptr;
        int pVideoYuvLen = -1;
        struct SwsContext *pRGBASwsCtx = nullptr;
        struct SwsContext *pNv21SwsCtx = nullptr;

        jmethodID mJpegCallBackId = nullptr;
        jmethodID mYuvCallBackId = nullptr;
        const char *mFileName = nullptr;

        double mVideoCurDuration = 0;
        double mVideoAllDuration = 0;

        int initData();

        void ResetInfo(VideoInfo *info) {
            mStretchMode = false;
            mPreviewWidth = 0;
            mPreviewHeight = 0;
            mPreRotation = 0;
            mVideoCurDuration = 0;
            mVideoAllDuration = 0;
            //视频原始尺寸
            info->video_width = 0;
            info->video_height = 0;
            info->video_rotation = ROTATION_0;
            //当前角度
            info->display_rotation = 0;
            //当前播放尺寸
            info->display_width = 0;
            info->display_height = 0;
            info->need_scale = false;

            //原始尺寸的坐标
            info->crop_x = 0;
            info->crop_y = 0;
            info->crop_width = 0;
            info->crop_height = 0;
            info->need_crop = false;

            //
            info->scale_width = 0;
            info->scale_height = 0;
            info->rotate_width = 0;
            info->rotate_height = 0;
        }
    };

}

#endif //FFMPEGNATIVEWINDOW_FFMPEG_H
