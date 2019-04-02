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

#define AUTO_ROTATION_FRAME 1

namespace kk {
    class VideoPlayer {
    public:
        VideoPlayer();

        void SetSurface(ANativeWindow *surface) {
            if (pNativeWindow != nullptr) {
                ANativeWindow_release(pNativeWindow);
                pNativeWindow = nullptr;
            }
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
        int mPlayWidth;
        int mPlayHeight;
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
        AVFrame *pRotationFrame = nullptr;
        AVRational pTimeBase;
        uint8_t *pRotateBuf = nullptr;
        uint8_t *pRgbaBuf = nullptr;
        uint8_t *pNv21Buf = nullptr;
        struct SwsContext *pRGBASwsCtx = nullptr;
        struct SwsContext *pNv21SwsCtx = nullptr;

        bool mSoftMode;

        jmethodID mCallBackId = nullptr;
        const char *mFileName = nullptr;
    };
}

#endif //FFMPEGNATIVEWINDOW_FFMPEG_H
