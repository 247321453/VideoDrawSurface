//
// Created by user on 2019/4/2.
//

#ifndef FFMPEGNATIVEWINDOW_FFMPEG_H
#define FFMPEGNATIVEWINDOW_FFMPEG_H

#include <jni.h>
#include <android/native_window.h>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
};

namespace kk {
    class VideoPlayer {
    public:
        VideoPlayer();

        void SetSurface(ANativeWindow *surface, int width, int height, bool stretch) {
            if (pNativeWindow != nullptr) {
                ANativeWindow_release(pNativeWindow);
                pNativeWindow = nullptr;
            }
            mStretchMode = stretch;
            mPreviewWidth = width;
            mPreviewHeight = height;
            pNativeWindow = surface;
        }

        void SetDataSource(const char *path) {
            Release();
            mPreLoad = false;
            mFileName = path;
        }

        void SetCallBack(jmethodID callback, bool needNv21Data) {
            mCallBackId = callback;
            mNeedNv21Data = needNv21Data;
        }

        int PreLoad();

        int Play(JNIEnv *env, jobject obj);

        void Stop();

        void Close();

        void Release();

        int Seek(double time);

        double GetPlayDuration() {
            return mVideoCurDuration;
        }

        double GetVideoDuration() {
            return mVideoAllDuration;
        }

        bool IsPlaying() {
            return mPlaying;
        }

        ~VideoPlayer() {
            Close();
        }

    private:
        bool mPreLoad;
        //循环播放
        bool mPlaying;
        bool mClose;
        int mRotation;
        int mVideoStream = -1;
        // 用户需要的尺寸
        int mPreviewWidth;
        int mPreviewHeight;
        //拉伸
        bool mStretchMode;
        int mRotateWidth;
        int mRotateHeight;
        //视频原始尺寸
        int mVideoWidth;
        int mVideoHeight;

        int mDisplayWidth;
        int mDisplayHeight;

        bool mNeedScaleCat;

        //原始尺寸的坐标
        int mCropLeft;
        int mCropTop;
        int mCropWidth;
        int mCropHeight;

        //
        int mScaleWidth;
        int mScaleHeight;

        double mVideoCurDuration;
        double mVideoAllDuration;
        bool mNeedNv21Data = false;
        ANativeWindow *pNativeWindow = nullptr;
        AVFormatContext *pFormatCtx = nullptr;
        AVCodecContext *pCodecCtx = nullptr;
        AVCodec *pCodec = nullptr;
        AVFrame *pFrameRGBA = nullptr;
        AVFrame *pFrameNv21 = nullptr;
        AVFrame *pFrame = nullptr;
        AVFrame *pRotateFrame = nullptr;
        AVFrame *pScaleFrame = nullptr;
        AVRational pTimeBase;
        uint8_t *pScaleBuf = nullptr;
        uint8_t *pRotateBuf = nullptr;
        uint8_t *pRgbaBuf = nullptr;
        uint8_t *pNv21Buf = nullptr;
        struct SwsContext *pRGBASwsCtx = nullptr;
        struct SwsContext *pNv21SwsCtx = nullptr;

        bool mSoftMode;

        jmethodID mCallBackId = nullptr;
        const char *mFileName = nullptr;

        void initSize();

        int initData();
    };
}

#endif //FFMPEGNATIVEWINDOW_FFMPEG_H
