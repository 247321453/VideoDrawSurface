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
            if (pNativeWindow != NULL) {
                ANativeWindow_release(pNativeWindow);
                pNativeWindow = NULL;
            }
            pNativeWindow = surface;
        }

        void SetDataSource(const char *path) {
            mPreLoad = false;
            mFileName = path;
        }

        void SetCallBack(jmethodID callback);

        int PreLoad();

        int Play(JNIEnv *env, jobject obj);

        void Stop();

        void Close();

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
        int mVideoStream;
        int mPlayWidth;
        int mPlayHeight;
        ANativeWindow *pNativeWindow = NULL;
        AVFormatContext *pFormatCtx = NULL;
        AVCodecContext *pCodecCtx = NULL;
        AVCodec *pCodec = NULL;
//        AVCodecParserContext *pParserCtx = NULL;
        AVFrame *pFrame = NULL;
        uint8_t *pNv21Data = NULL;
        uint8_t *pRgbaData = NULL;
        bool mSoftMode;

        void OnCallBack(JNIEnv *env, jobject obj, uint8_t *nv21Data, int width, int height, int yuvSize);

        jmethodID mCallBackId = nullptr;
        const char *mFileName = NULL;
    };
}

#endif //FFMPEGNATIVEWINDOW_FFMPEG_H
