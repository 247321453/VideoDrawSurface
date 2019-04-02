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

void VideoPlayer::Close() {
    mClose = true;
    Stop();
    mCallBackId = NULL;
    if (rBuf != NULL) {
        av_free(rBuf);
    }
    if (rgbaBuf != NULL) {
        av_free(rgbaBuf);
    }
    if (nv21Buf != NULL) {
        av_free(nv21Buf);
    }
    if (pRotationFrame != NULL) {
        av_free(pRotationFrame);
    }
    if (pFrameRGBA != NULL) {
        av_free(pFrameRGBA);
    }
    if (pFrameNv21 != NULL) {
        av_free(pFrameNv21);
    }
    if (pFrame != NULL) {
        av_free(pFrame);
    }
    if (pNativeWindow != NULL) {
        ANativeWindow_release(pNativeWindow);
        pNativeWindow = NULL;
    }
    //ffmpeg
//    if (pParserCtx != NULL) {
//        av_parser_close(pParserCtx);
//    }
    if (pCodecCtx != NULL) {
        avcodec_close(pCodecCtx);
    }
    if (pFormatCtx != NULL) {
        avformat_free_context(pFormatCtx);
    }
}

void VideoPlayer::Stop() {
    mPlaying = false;
}

/*
 * YUV420p：又叫planer平面模式，Y ，U，V分别再不同平面，也就是有三个平面。
 * YUV420p又分为：他们的区别只是存储UV的顺序不一样而已。
 * I420:又叫YU12，安卓的模式。存储顺序是先存Y，再存U，最后存V。YYYYUUUVVV
 * YV12:存储顺序是先存Y，再存V，最后存U。YYYVVVUUU
 *
 * YUV420sp：又叫bi-planer或two-planer双平面，Y一个平面，UV在同一个平面交叉存储。
 * YUV420sp又分为：他们的区别只是存储UV的顺序不一样而已。
 * NV12:IOS只有这一种模式。存储顺序是先存Y，再UV交替存储。YYYYUVUVUV
 * NV21:安卓的模式。存储顺序是先存Y，再存U，再VU交替存储。YYYYVUVUVU
 */
void VideoPlayer::OnCallBack(JNIEnv *env, jobject obj, AVFrame *frameNv21, int len, int width,
                             int height) {
    if (mCallBackId != nullptr) {
        uint8_t *yuvBuf = new uint8_t[len];
        size_t y_step = width * height * sizeof(uint8_t);
        memcpy(yuvBuf, frameNv21->data[0], y_step);//拷贝Y分量
        memcpy(yuvBuf + y_step, frameNv21->data[1], y_step / 2);//调试到这里,data[2]没有值

        jbyte *data = (jbyte *) yuvBuf;
        jbyteArray array = env->NewByteArray(len);
        if (array != NULL) {
            env->SetByteArrayRegion(array, 0, len, data);
            env->CallVoidMethod(obj, mCallBackId, array, width, height);
        } else {
            ALOGW("new byte array failed");
        }
    }
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

    mPlaying = true;
    AVPacket packet;
    ANativeWindow_Buffer windowBuffer;

    if (pFrame == NULL) {
        pFrame = av_frame_alloc();
    }

    if (pRotationFrame == NULL) {
        pRotationFrame = av_frame_alloc();
        int numBytes = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, mPlayWidth, mPlayHeight, 1);

        rBuf = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

        av_image_fill_arrays(pRotationFrame->data, pRotationFrame->linesize,
                             rBuf, AV_PIX_FMT_YUV420P,
                             mPlayWidth, mPlayHeight, 1);
    }

    if (pNativeWindow != NULL) {
        if (pFrameRGBA == NULL) {
            pFrameRGBA = av_frame_alloc();
        }
        if (pRGBASwsCtx == nullptr) {
            int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, mPlayWidth, mPlayHeight, 1);

            rgbaBuf = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

            av_image_fill_arrays(pFrameRGBA->data, pFrameRGBA->linesize,
                                 rgbaBuf, AV_PIX_FMT_RGBA,
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

    if (mCallBackId != nullptr) {
        if (pFrameNv21 == NULL) {
            pFrameNv21 = av_frame_alloc();
        }
        if (pNv21SwsCtx == nullptr) {
            int numBytes = av_image_get_buffer_size(AV_PIX_FMT_NV21, mPlayWidth, mPlayHeight, 1);

            nv21Buf = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

            av_image_fill_arrays(pFrameNv21->data, pFrameNv21->linesize,
                                 nv21Buf, AV_PIX_FMT_NV21,
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
    long time, cur;
    int yuvSize = mPlayWidth * mPlayHeight * 3 / 2;
    ALOGD("start av_read_frame, width=%d,height=%d", mPlayWidth, mPlayHeight);
    while (av_read_frame(pFormatCtx, &packet) >= 0) {
        if (!mPlaying) {
            break;
        }
        //use the parser to split the data into frames
        if (packet.stream_index == mVideoStream) {
            ret = avcodec_send_packet(pCodecCtx, &packet);

            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                return -9;
            }

            while (avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
                if (!mPlaying) {
                    break;
                }
                time = getCurTime();
                if (AUTO_ROTATION_FRAME == 0 || mRotation == ROTATION_0) {
                    tmpFrame = pFrame;
                } else {
                    av_frame_rotate(pFrame, mRotation, pRotationFrame);
                    tmpFrame = pRotationFrame;
                }
                cur = getCurTime();
                ALOGD("yuv rotate use time %ld", (cur - time));
                time = cur;
                if (pNativeWindow != NULL) {
                    sws_scale(pRGBASwsCtx, (uint8_t const *const *) tmpFrame->data,
                              tmpFrame->linesize, 0, mPlayHeight,
                              pFrameRGBA->data, pFrameRGBA->linesize);
                    cur = getCurTime();
                    ALOGD("sws_scale rgba use time %ld", (cur - time));
                    time = cur;
                    if (ANativeWindow_lock(pNativeWindow, &windowBuffer, NULL) >= 0) {
                        uint8_t *dst = (uint8_t *) windowBuffer.bits;
                        int dstStride = windowBuffer.stride * 4;
                        uint8_t *src = (uint8_t *) pFrameRGBA->data[0];
                        size_t srcStride = (size_t) pFrameRGBA->linesize[0];
                        // 由于window的stride和帧的stride不同,因此需要逐行复制
                        for (h = 0; h < mPlayHeight; h++) {
                            memcpy(dst + h * dstStride, src + h * srcStride, srcStride);
                        }
                        ANativeWindow_unlockAndPost(pNativeWindow);
                    }
                }
                if (mCallBackId != nullptr) {
                    sws_scale(pNv21SwsCtx, (uint8_t const *const *) tmpFrame->data,
                              tmpFrame->linesize, 0, mPlayHeight,
                              pFrameNv21->data, pFrameNv21->linesize);
                    cur = getCurTime();
                    ALOGD("sws_scale nv21 use time %ld", (cur - time));
                    time = cur;
                    OnCallBack(env, obj, pFrameNv21, yuvSize, mPlayWidth, mPlayHeight);
                }
            }
            if (ret < 0 && ret != AVERROR_EOF) {
                return -8;
            }
        }
        av_packet_unref(&packet);
    }
    ALOGD("end av_read_frame");
    mPlaying = false;
    return 0;
}

void VideoPlayer::SetCallBack(jmethodID callback) {
    mCallBackId = callback;
    ALOGD("SetCallBack");
}

int VideoPlayer::PreLoad() {
    if (mPreLoad) {
        return 0;
    }
    ALOGD("PreLoad");
    mPreLoad = true;
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


//    pCodecCtx = pFormatCtx->streams[mVideoStream]->codec;//deprecated
    pCodecCtx = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[mVideoStream]->codecpar);
    //声明位于libavcodec/avcodec.h

    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);//用于查找FFmpeg的解码器。
    //函数的参数是一个解码器的ID，返回查找到的解码器（没有找到就返回NULL）

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
    ALOGD("PreLoad::ok");
    return 0;
}
