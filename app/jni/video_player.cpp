//
// Created by user on 2019/4/2.
//

#include "video_player.h"
#include "av_rotation.h"
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
    if (nativeWindow != NULL) {
        ANativeWindow_release(nativeWindow);
        nativeWindow = NULL;
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

struct SwsContext *
VideoPlayer::createRGBASwsContext(AVPixelFormat src_fmt, int src_width, int src_height,
                                  AVFrame *frameRGBA, int width, int height) {
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA, width, height, 1);

    uint8_t *buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

    av_image_fill_arrays(frameRGBA->data, frameRGBA->linesize,
                         buffer, AV_PIX_FMT_RGBA,
                         width, height, 1);
    struct SwsContext *sws_ctx = sws_getContext(src_width,
                                                src_height,
                                                src_fmt,
                                                width,
                                                height,
                                                AV_PIX_FMT_RGBA,
                                                SWS_BILINEAR,
                                                NULL,
                                                NULL,
                                                NULL);
    return sws_ctx;
}

struct SwsContext *
VideoPlayer::createNv21SwsContext(AVPixelFormat src_fmt, int src_width, int src_height,
                                  AVFrame *frameNv21, int width, int height) {
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_NV21, width, height, 1);

    uint8_t *buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

    av_image_fill_arrays(frameNv21->data, frameNv21->linesize,
                         buffer, AV_PIX_FMT_NV21,
                         width, height, 1);
    struct SwsContext *sws_ctx = sws_getContext(src_width,
                                                src_height,
                                                src_fmt,
                                                width,
                                                height,
                                                AV_PIX_FMT_NV21,
                                                SWS_FAST_BILINEAR,
                                                NULL,
                                                NULL,
                                                NULL);
    return sws_ctx;
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
void VideoPlayer::OnCallBack(JNIEnv *env, jobject obj, AVFrame *frameNv21, int width, int height) {
    if (mCallBackId != nullptr) {
        int framelength = width * height;
        int len = framelength * 3 / 2;
        size_t Ystep = framelength * sizeof(uint8_t);

        uint8_t *yData = frameNv21->data[0];
        uint8_t *vuData = frameNv21->data[1];

        ALOGD("all=%d, yData:len=%d, vuData:len=%d", len,
              sizeof(yData) / sizeof(yData[0]),
              sizeof(vuData) / sizeof(vuData[0]));
//
//        uint8_t *yuvData = new uint8_t[len];
//        memcpy(yuvData, yData, Ystep);//拷贝Y分量
//        memcpy(yuvData + Ystep, frameNv21->data[1], Ystep / 2);//vu
//
//        jbyte *data = (jbyte *) yuvData;
//        jbyteArray array = env->NewByteArray(len);
//        ALOGD("yuv:len=%d, size=%d", len, (width * height * 3 / 2));
//        env->SetByteArrayRegion(array, 0, len, data);
//        env->CallVoidMethod(obj, mCallBackId, array);
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
    ALOGD("Play:rotation=%d, width=%d,height=%d", mRotation, pCodecCtx->width, pCodecCtx->height);
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

        uint8_t *buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

        av_image_fill_arrays(pRotationFrame->data, pRotationFrame->linesize,
                             buffer, AV_PIX_FMT_YUV420P,
                             mPlayWidth, mPlayHeight, 1);
    }
    if (nativeWindow != NULL) {
        if (pFrameRGBA == NULL) {
            pFrameRGBA = av_frame_alloc();
        }
        if (pRGBASwsCtx == nullptr) {
            pRGBASwsCtx = createRGBASwsContext(pCodecCtx->pix_fmt, mPlayWidth, mPlayHeight,
                                               pFrameRGBA,
                                               mPlayWidth, mPlayHeight);
        }
    }
    if (mCallBackId != nullptr) {
        if (pFrameNv21 == NULL) {
            pFrameNv21 = av_frame_alloc();
        }
        if (pNv21SwsCtx == nullptr) {
            pNv21SwsCtx = createNv21SwsContext(pCodecCtx->pix_fmt, mPlayWidth, mPlayHeight,
                                               pFrameRGBA,
                                               mPlayWidth, mPlayHeight);
        }
    }
    int h;
    AVFrame *tmpFrame;
    ANativeWindow_setBuffersGeometry(nativeWindow, mPlayWidth, mPlayHeight,
                                     WINDOW_FORMAT_RGBA_8888);
    ALOGD("start av_read_frame, width=%d,height=%d", mPlayWidth, mPlayHeight);
    while (av_read_frame(pFormatCtx, &packet) >= 0) {
        if (!mPlaying) {
            break;
        }
        /* use the parser to split the data into frames */
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
                if (AUTO_ROTATION_FRAME == 0 || mRotation == ROTATION_0) {
                    tmpFrame = pFrame;
                } else if (mRotation == ROTATION_90) {
                    av_frame_rotate_90(pFrame, pRotationFrame);
                    tmpFrame = pRotationFrame;
                } else if (mRotation == ROTATION_180) {
                    av_frame_rotate_180(pFrame, pRotationFrame);
                    tmpFrame = pRotationFrame;
                } else if (mRotation == ROTATION_270) {
                    av_frame_rotate_270(pFrame, pRotationFrame);
                    tmpFrame = pRotationFrame;
                } else {
                    tmpFrame = pFrame;
                }
                if (nativeWindow != NULL) {
                    sws_scale(pRGBASwsCtx, (uint8_t const *const *) tmpFrame->data,
                              tmpFrame->linesize, 0, mPlayHeight,
                              pFrameRGBA->data, pFrameRGBA->linesize);
                    if (ANativeWindow_lock(nativeWindow, &windowBuffer, NULL) >= 0) {
                        uint8_t *dst = (uint8_t *) windowBuffer.bits;
//                        uint8_t *src = (uint8_t *) pFrameRGBA->data[0];
//                        size_t len = static_cast<size_t>(4 * mPlayWidth * mPlayHeight);
//                        memcpy(dst, src, len);
                        int dstStride = windowBuffer.stride * 4;
                        uint8_t *src = (uint8_t *) pFrameRGBA->data[0];
                        size_t srcStride = (size_t) pFrameRGBA->linesize[0];
                        // 由于window的stride和帧的stride不同,因此需要逐行复制
                        for (h = 0; h < mPlayHeight; h++) {
                            memcpy(dst + h * dstStride, src + h * srcStride, srcStride);
                        }
                        ANativeWindow_unlockAndPost(nativeWindow);
                    }
                }
                if (mCallBackId != nullptr) {
                    sws_scale(pNv21SwsCtx, (uint8_t const *const *) tmpFrame->data,
                              tmpFrame->linesize, 0, mPlayHeight,
                              pFrameNv21->data, pFrameNv21->linesize);
                    OnCallBack(env, obj, pFrameNv21, mPlayWidth, mPlayHeight);
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

    //pCodecCtx = pFormatCtx->streams[mVideoStream]->codec;//deprecated
    pCodecCtx = avcodec_alloc_context3(NULL);

    avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[mVideoStream]->codecpar);

    //声明位于libavcodec/avcodec.h
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);//用于查找FFmpeg的解码器。
    //函数的参数是一个解码器的ID，返回查找到的解码器（没有找到就返回NULL）

//    pParserCtx = av_parser_init(pCodec->id);

//    if (!pCodecCtx) {
//        return -5;
//    }

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        return -6;
    }
    ALOGD("PreLoad::ok");
    return 0;
}
