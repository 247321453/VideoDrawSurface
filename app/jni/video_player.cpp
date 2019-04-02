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
    if (pNv21Data != NULL) {
        free(pNv21Data);
    }
    if (pRgbaData != NULL) {
        free(pRgbaData);
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
void VideoPlayer::OnCallBack(JNIEnv *env, jobject obj, uint8_t *nv21Data, int width, int height, int yuvSize) {
    if (mCallBackId != nullptr) {
        jbyte *data = (jbyte *) nv21Data;
        jbyteArray array = env->NewByteArray(yuvSize);
        if(array != NULL){
            env->SetByteArrayRegion(array, 0, yuvSize, data);
            env->CallVoidMethod(obj, mCallBackId, array, width, height);
        }else{
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
    int videoWidth = pCodecCtx->width;
    int videoHeight = pCodecCtx->height;

    if (pFrame == NULL) {
        pFrame = av_frame_alloc();
    }
    int rgbaSize = 4 * videoWidth * videoHeight;
    if (pNativeWindow != NULL) {
        if (pRgbaData == NULL) {
            pRgbaData = (uint8_t *) malloc(sizeof(uint8_t) * rgbaSize);
        }
    }
    int yuvSize = mPlayWidth * mPlayHeight * 3 / 2;
    if (mCallBackId != nullptr) {
        if (pNv21Data == NULL) {
            pNv21Data = (uint8_t *) malloc(sizeof(uint8_t) * yuvSize);
        }
    }
    int h;
    AVFrame *tmpFrame;
    ANativeWindow_setBuffersGeometry(pNativeWindow, mPlayWidth, mPlayHeight,
                                     WINDOW_FORMAT_RGBA_8888);

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

            while (avcodec_receive_frame(pCodecCtx, pFrame) == 0)//解码出来的frame?
            {
                if (!mPlaying) {
                    break;
                }
                ret = convert(pFrame, mRotation, videoWidth, videoHeight, pNv21Data, pRgbaData);
                if (ret != 0) {
                    ALOGE("convert error:%d", ret);
                    break;
                }
                if (pNativeWindow != NULL) {
                    if (ANativeWindow_lock(pNativeWindow, &windowBuffer, NULL) == 0) {
                        //memcpy((uint8_t *) windowBuffer.bits, pRgbaData, rgbaSize);
                        uint8_t *dst = (uint8_t *) windowBuffer.bits;
                        int dstStride = windowBuffer.stride * 4;
                        uint8_t *src = pRgbaData;
                        size_t src_stride = static_cast<size_t>(mPlayWidth * 4);
                        // 由于window的stride和帧的stride不同,因此需要逐行复制
                        for (h = 0; h < mPlayHeight; h++) {
                            memcpy(dst + h * dstStride, src + h * src_stride, src_stride);
                        }
                        ANativeWindow_unlockAndPost(pNativeWindow);
                    } else {
                        break;
                    }
                }
                if (mCallBackId != nullptr) {
                    OnCallBack(env, obj, pNv21Data, mPlayWidth, mPlayHeight, yuvSize);
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
