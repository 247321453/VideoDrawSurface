//
// Created by user on 2019/4/2.
//

#include <android/native_window_jni.h>
#include "video_player.h"
#include "util.h"
#include "debug.h"

using namespace kk;

VideoPlayer::VideoPlayer() {
    mClose = true;
    mPlaying = false;
    mRotation = ROTATION_0;
}

void VideoPlayer::Stop() {
    mPlaying = false;
}

int VideoPlayer::Play(JNIEnv *env, jobject obj) {
//    if (pParserCtx == NULL) {
//        return -10;
//    }
    if (mPlaying) {
        return 1;
    }
    int ret = PreLoad();
    if (ret != 0) {
        return ret;
    }
    //角度
    mRotation = av_get_rotation(pFormatCtx->streams[mVideoStream]);
    ALOGD("video:rotation=%d, width=%d,height=%d", mRotation, pCodecCtx->width, pCodecCtx->height);
    if (AUTO_ROTATION_FRAME != 0 && (mRotation == ROTATION_90 || mRotation == ROTATION_270)) {
        mPlayWidth = pCodecCtx->height;
        mPlayHeight = pCodecCtx->width;
    } else {
        mPlayWidth = pCodecCtx->width;
        mPlayHeight = pCodecCtx->height;
    }
    int yuvLen = mPlayWidth * mPlayHeight * 3 / 2;
    size_t y_step = mPlayWidth * mPlayHeight * sizeof(uint8_t);

    mPlaying = true;
    AVPacket packet;
    ANativeWindow_Buffer windowBuffer;

    if (pFrame == nullptr) {
        pFrame = av_frame_alloc();
    }

    if (pRotationFrame == nullptr) {
        pRotationFrame = av_frame_alloc();
        int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, mPlayWidth, mPlayHeight, 1);

        pRotateBuf = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

        av_image_fill_arrays(pRotationFrame->data, pRotationFrame->linesize,
                             pRotateBuf, AV_PIX_FMT_YUV420P,
                             mPlayWidth, mPlayHeight, 1);
    }

    if (pNativeWindow != nullptr) {
        if (pFrameRGBA == nullptr) {
            pFrameRGBA = av_frame_alloc();
        }
        if (pRGBASwsCtx == nullptr) {
            int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, mPlayWidth, mPlayHeight, 1);

            pRgbaBuf = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

            av_image_fill_arrays(pFrameRGBA->data, pFrameRGBA->linesize,
                                 pRgbaBuf, AV_PIX_FMT_RGBA,
                                 mPlayWidth, mPlayHeight, 1);
            pRGBASwsCtx = sws_getContext(mPlayWidth,
                                         mPlayHeight,
                                         pCodecCtx->pix_fmt,
                                         mPlayWidth,
                                         mPlayHeight,
                                         AV_PIX_FMT_RGBA,
                                         SWS_BILINEAR,
                                         NULL,
                                         NULL,
                                         NULL);
        }
    }

    jbyte *yuvNv21Data = nullptr;
    jbyteArray yuvArray = nullptr;
    if (mNeedNv21Data) {
        yuvNv21Data = new jbyte[yuvLen];
        yuvArray = env->NewByteArray(yuvLen);
        if (pFrameNv21 == nullptr) {
            pFrameNv21 = av_frame_alloc();
        }
        if (pNv21SwsCtx == nullptr) {
            int numBytes = av_image_get_buffer_size(AV_PIX_FMT_NV21, mPlayWidth, mPlayHeight, 1);

            pNv21Buf = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

            av_image_fill_arrays(pFrameNv21->data, pFrameNv21->linesize,
                                 pNv21Buf, AV_PIX_FMT_NV21,
                                 mPlayWidth, mPlayHeight, 1);
            pNv21SwsCtx = sws_getContext(mPlayWidth,
                                         mPlayHeight,
                                         pCodecCtx->pix_fmt,
                                         mPlayWidth,
                                         mPlayHeight,
                                         AV_PIX_FMT_NV21,
                                         SWS_FAST_BILINEAR,
                                         NULL,
                                         NULL,
                                         NULL);
        }
    }
    int h;
    AVFrame *tmpFrame;
    ANativeWindow_setBuffersGeometry(pNativeWindow, mPlayWidth, mPlayHeight,
                                     WINDOW_FORMAT_RGBA_8888);
//    long time, cur;
    bool error = false;
    ALOGD("start av_read_frame, width=%d,height=%d", mPlayWidth, mPlayHeight);
    while (mPlaying && av_read_frame(pFormatCtx, &packet) >= 0) {
        //use the parser to split the data into frames
        if (packet.stream_index == mVideoStream) {
            ret = avcodec_send_packet(pCodecCtx, &packet);

            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                error = true;
                break;
            }

            while (mPlaying && avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
//                time = getCurTime();
                mVideoCurDuration = pFrame->pts * av_q2d(pTimeBase);
                if (AUTO_ROTATION_FRAME == 0 || mRotation == ROTATION_0) {
                    tmpFrame = pFrame;
                } else {
                    av_frame_rotate(pFrame, mRotation, pRotationFrame);
                    tmpFrame = pRotationFrame;
                }
                //TODO 缩放，裁剪
//                cur = getCurTime();
//                ALOGD("yuv rotate use time %ld", (cur - time));
//                time = cur;
                //surface绘制
                if (pNativeWindow != nullptr) {
                    ALOGD("draw surface");
                    sws_scale(pRGBASwsCtx, (uint8_t const *const *) tmpFrame->data,
                              tmpFrame->linesize, 0, mPlayHeight,
                              pFrameRGBA->data, pFrameRGBA->linesize);
//                    cur = getCurTime();
//                    ALOGD("sws_scale rgba use time %ld", (cur - time));
//                    time = cur;
                    if (ANativeWindow_lock(pNativeWindow, &windowBuffer, NULL) >= 0) {
                        uint8_t *dst = (uint8_t *) windowBuffer.bits;
                        int dstStride = windowBuffer.stride * 4;
                        uint8_t *src = (uint8_t *) pFrameRGBA->data[0];
                        size_t src_stride = (size_t) pFrameRGBA->linesize[0];
                        // 由于window的stride和帧的stride不同,因此需要逐行复制
                        for (h = 0; h < mPlayHeight; h++) {
                            memcpy(dst + h * dstStride, src + h * src_stride, src_stride);
                        }
                        ANativeWindow_unlockAndPost(pNativeWindow);
                    }
                }
                if (mCallBackId != nullptr) {
                    if(mNeedNv21Data){
                        ALOGD("callback with data");
                        sws_scale(pNv21SwsCtx, (uint8_t const *const *) tmpFrame->data,
                                  tmpFrame->linesize, 0, mPlayHeight,
                                  pFrameNv21->data, pFrameNv21->linesize);
                        if (yuvNv21Data != nullptr && yuvArray != nullptr) {
                            memcpy(yuvNv21Data, pFrameNv21->data[0], y_step);//拷贝Y分量
                            memcpy(yuvNv21Data + y_step, pFrameNv21->data[1], y_step / 2);//uv
                            env->SetByteArrayRegion(yuvArray, 0, yuvLen, yuvNv21Data);
                            env->CallVoidMethod(obj, mCallBackId, yuvArray, mPlayWidth,
                                                mPlayHeight, mVideoCurDuration, mVideoAllDuration);
                        }
                    } else {
                        ALOGD("callback");
                        env->CallVoidMethod(obj, mCallBackId, NULL, mPlayWidth,
                                            mPlayHeight, mVideoCurDuration, mVideoAllDuration);
                    }
//                    cur = getCurTime();
//                    ALOGD("sws_scale nv21 use time %ld", (cur - time));
//                    time = cur;
                }
            }
            if (ret < 0 && ret != AVERROR_EOF) {
                error =true;
                break;
            }
        }
        av_packet_unref(&packet);
    }
    ALOGD("end av_read_frame");
    if(yuvArray != nullptr && yuvNv21Data != nullptr){
        env->ReleaseByteArrayElements(yuvArray, yuvNv21Data, JNI_COMMIT);
    }
    ALOGD("free arr ok");
    if(yuvNv21Data != nullptr){
        free(yuvNv21Data);
    }
    ALOGD("free data ok");
    mPlaying = false;
    if(error){
        return ret;
    }
    return 0;
}

int VideoPlayer::PreLoad() {
    if (mPreLoad) {
        return 0;
    }
    ALOGD("PreLoad");
    mPreLoad = false;
    pFormatCtx = avformat_alloc_context();
    int result = 0;
    result = avformat_open_input(&pFormatCtx, mFileName, NULL, NULL);
    if (result != 0) {
        return -2;
    }

    result = avformat_find_stream_info(pFormatCtx, NULL);
    if (result != 0) {
        return -3;
    }

    //find videostream data
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            mVideoStream = i;
            break;
        }
    }
    if (mVideoStream == -1) {
        return -4;
    }
    AVStream *steam = pFormatCtx->streams[mVideoStream];
    AVRational time_base = steam->time_base;
    pTimeBase.den = time_base.den;
    pTimeBase.num = time_base.num;
    if(pFormatCtx->duration != AV_NOPTS_VALUE){
        mVideoAllDuration = steam->duration * av_q2d(time_base);
        ALOGD("PreLoad:all time=%f", mVideoAllDuration);
    }
    pCodecCtx = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(pCodecCtx, steam->codecpar);
    //声明位于libavcodec/avcodec.h
    mSoftMode = true;
    ALOGD("code::default");
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);//软解
    if (pCodec == NULL) {
        return -5;
    }
//    pParserCtx = av_parser_init(pCodec->id);
//
//    if (!pCodecCtx) {
//        return -5;
//    }

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        return -6;
    }
    mPreLoad = true;
    ALOGD("PreLoad::ok");
    return 0;
}

void VideoPlayer::Close() {
    mPlaying = false;
    mClose = true;
    mCallBackId = nullptr;
    if (pNativeWindow != nullptr) {
        ANativeWindow_release(pNativeWindow);
        pNativeWindow = nullptr;
    }
    Release();
}

void VideoPlayer::Release() {
    if (pRotateBuf != nullptr) {
        av_free(pRotateBuf);
        pRotateBuf = nullptr;
    }
    if (pRgbaBuf != nullptr) {
        av_free(pRgbaBuf);
        pRgbaBuf = nullptr;
    }
    if (pNv21Buf != nullptr) {
        av_free(pNv21Buf);
        pNv21Buf = nullptr;
    }
    if (pRotationFrame != nullptr) {
        av_free(pRotationFrame);
        pRotationFrame = nullptr;
    }
    if (pFrameRGBA != nullptr) {
        av_free(pFrameRGBA);
        pFrameRGBA = nullptr;
    }
    if (pFrameNv21 != nullptr) {
        av_free(pFrameNv21);
        pFrameNv21 = nullptr;
    }
    if (pFrame != nullptr) {
        av_free(pFrame);
        pFrame = nullptr;
    }
    if (pCodecCtx != nullptr) {
        avcodec_close(pCodecCtx);
        pCodecCtx = nullptr;
    }
    if (pFormatCtx != nullptr) {
        avformat_free_context(pFormatCtx);
        pFormatCtx = nullptr;
    }
}

int VideoPlayer::Seek(double ms) {
    if (mPreLoad && pFormatCtx != NULL && mVideoStream != -1) {
        //ms * AV_TIME_BASE / 1000
        return av_seek_frame(pFormatCtx, mVideoStream, static_cast<int64_t>(ms * AV_TIME_BASE), AVSEEK_FLAG_BACKWARD);
    }
    return -1;
}
