//
// Created by user on 2019/4/2.
//

#ifndef KKPLAYER_FFMPEG_H
#define KKPLAYER_FFMPEG_H

#include <jni.h>
#include <android/native_window.h>
#include "util.h"
#include "debug.h"
#include "size_info.h"

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

        void SetSize(int width, int height, bool stretch, int preRotation) {
            mStretchMode = stretch;
            mPreviewWidth = width;
            mPreviewHeight = height;
            mPreRotation = preRotation;
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

        SizeInfo GetVideoInfo() {
            return Info;
        }

        bool IsPlaying() {
            return mPlaying;
        }

        int TakeImage(JNIEnv *env, jobject obj, int width, int height, int rotation, bool mirror);

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

        SizeInfo Info;

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

        void ResetInfo(SizeInfo *info) {
            mStretchMode = false;
            mPreviewWidth = 0;
            mPreviewHeight = 0;
            mPreRotation = 0;
            mVideoCurDuration = 0;
            mVideoAllDuration = 0;
            ResetSizeInfo(info);
        }
    };

}

#endif //KKPLAYER_FFMPEG_H
