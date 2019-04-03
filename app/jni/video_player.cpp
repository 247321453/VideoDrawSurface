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

uint8_t *initFrame(AVFrame *frame, AVPixelFormat video_fmt, int width, int height) {
    int count = av_image_get_buffer_size(video_fmt, width, height, 1);
    uint8_t *buf = (uint8_t *) av_malloc(count * sizeof(uint8_t));
    av_image_fill_arrays(frame->data, frame->linesize,
                         buf, video_fmt, width, height, 1);
    return buf;
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
        ALOGW("open file fail %s", mFileName);
        return -2;
    }

    result = avformat_find_stream_info(pFormatCtx, NULL);
    if (result != 0) {
        return -3;
    }

    //find videostream data
    AVStream *steam = nullptr;
    for (int i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            mVideoStream = i;
            steam = pFormatCtx->streams[i];
            break;
        }
    }
    if (steam == nullptr) {
        return -4;
    }
    AVRational time_base = steam->time_base;
    pTimeBase.den = time_base.den;
    pTimeBase.num = time_base.num;
    if (pFormatCtx->duration != AV_NOPTS_VALUE) {
        mVideoAllDuration = steam->duration * av_q2d(time_base);
        ALOGD("video:all time=%f", mVideoAllDuration);
    }
    //角度，宽高
    mRotation = av_get_rotation(steam);
    pCodecCtx = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(pCodecCtx, steam->codecpar);
    mVideoWidth = pCodecCtx->width;
    mVideoHeight = pCodecCtx->height;
    ALOGD("code::default");
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        return -5;
    }
    //软解
    mSoftMode = true;
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        return -6;
    }
    initSize();
    int ret = initData();
    if (ret != 0) {
        return ret;
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
    if (pScaleBuf != nullptr) {
        av_free(pScaleBuf);
        pScaleBuf = nullptr;
    }
    if (pScaleFrame != nullptr) {
        av_free(pScaleFrame);
        pScaleFrame = nullptr;
    }
    if (pRotateBuf != nullptr) {
        av_free(pRotateBuf);
        pRotateBuf = nullptr;
    }
    if (pRotateFrame != nullptr) {
        av_free(pRotateFrame);
        pRotateFrame = nullptr;
    }
    if (pRgbaBuf != nullptr) {
        av_free(pRgbaBuf);
        pRgbaBuf = nullptr;
    }
    if (pNv21Buf != nullptr) {
        av_free(pNv21Buf);
        pNv21Buf = nullptr;
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
        return av_seek_frame(pFormatCtx, mVideoStream, static_cast<int64_t>(ms * AV_TIME_BASE),
                             AVSEEK_FLAG_BACKWARD);
    }
    return -1;
}

void VideoPlayer::initSize() {
    ALOGD("video:rotation=%d, width=%d,height=%d", mRotation, mVideoWidth, mVideoHeight);
    if (mSoftMode && (mRotation == ROTATION_90 || mRotation == ROTATION_270)) {
        mRotateWidth = mVideoHeight;
        mRotateHeight = mVideoWidth;
    } else {
        //硬解，自动旋转角度
        mRotateWidth = mVideoWidth;
        mRotateHeight = mVideoHeight;
    }
    mDisplayWidth = mRotateWidth;
    mDisplayHeight = mRotateHeight;
    if (mPreviewWidth > 0 && mPreviewHeight > 0) {
        //TODO 计算缩放，裁剪
        if (mPreviewWidth == mDisplayWidth && mPreviewHeight == mDisplayHeight) {
            mNeedScaleCat = false;
        }
        //默认
        mNeedScaleCat = true;
        mCropTop = 0;
        mCropLeft = 0;
        mCropWidth = mDisplayWidth;
        mCropHeight = mDisplayHeight;
        mScaleWidth = mPreviewWidth;
        mScaleHeight = mPreviewHeight;
        if (!mStretchMode) {
            if ((((float) mPreviewWidth / (float) mPreviewHeight) ==
                 ((float) mDisplayWidth / (float) mDisplayHeight))) {
                //一样的比例
            }
            //目标的宽和原始比例的宽一样
            int w1 = mDisplayWidth;//540/360*640=960
            int h1 = (int) round((float) w1 / (float) mPreviewWidth * (float) mPreviewHeight);
            //目标的高和原始比例的高一样
            int h2 = mDisplayHeight;//960/640*360=540
            int w2 = (int) round(h2 / (float) mPreviewHeight * (float) mPreviewWidth);

            if (h1 < mDisplayHeight) {
                //裁剪的高度小于原始高度，进行裁剪
                mCropLeft = 0;
                mCropTop = (mDisplayHeight - h1) / 4 * 2;//不能单数 26/2=13  26/4*2=12
                mCropWidth = w1;
                mCropHeight = h1;
            } else if (w2 < mDisplayWidth) {
                //裁剪的宽度小于原始宽度，进行裁剪
                //宽度比目标宽
                mCropLeft = (mDisplayHeight - w2) / 4 * 2;
                mCropTop = 0;
                mCropWidth = w2;
                mCropHeight = h2;
            }
        }
    } else {
        mNeedScaleCat = false;
    }
    if (mNeedScaleCat) {
        mRotateWidth = mCropWidth;
        mRotateHeight = mCropHeight;
        mDisplayWidth = mScaleWidth;
        mDisplayHeight = mScaleHeight;
    }
}

int VideoPlayer::initData() {
    //原始
    if (pFrame == nullptr) {
        pFrame = av_frame_alloc();
    }
    AVPixelFormat video_fmt = pCodecCtx->pix_fmt;
    if (video_fmt != AV_PIX_FMT_YUV420P) {
        return -7;
    }

    if (mRotation != ROTATION_0 || mNeedScaleCat) {
        if (pRotateFrame == nullptr) {
            pRotateFrame = av_frame_alloc();
            pRotateBuf = initFrame(pRotateFrame, video_fmt, mRotateWidth, mRotateHeight);
            ALOGD("init rotate frame %dx%d", mRotateWidth, mRotateHeight);
        }
    }
    if (mNeedScaleCat) {
        if (pScaleFrame == nullptr) {
            pScaleFrame = av_frame_alloc();
            pScaleBuf = initFrame(pScaleFrame, video_fmt, mScaleWidth, mScaleHeight);
            ALOGD("init scale frame %dx%d", mScaleWidth, mScaleHeight);
        }
    }

    if (pNativeWindow != nullptr) {
        if (pFrameRGBA == nullptr) {
            pFrameRGBA = av_frame_alloc();
            pRgbaBuf = initFrame(pFrameRGBA, AV_PIX_FMT_RGBA, mDisplayWidth, mDisplayHeight);
            ALOGD("init rgba frame %dx%d", mDisplayWidth, mDisplayHeight);
        }
        if (pRGBASwsCtx == nullptr) {
            pRGBASwsCtx = sws_getContext(mDisplayWidth, mDisplayHeight, video_fmt,
                                         mDisplayWidth, mDisplayHeight, AV_PIX_FMT_RGBA,
                                         SWS_FAST_BILINEAR, NULL, NULL, NULL);
        }
    }

    if (mNeedNv21Data) {
        if (pFrameNv21 == nullptr) {
            pFrameNv21 = av_frame_alloc();
            pNv21Buf = initFrame(pFrameNv21, AV_PIX_FMT_NV21, mDisplayWidth, mDisplayHeight);
            ALOGD("init nv21 frame %dx%d", mDisplayWidth, mDisplayHeight);
        }
        if (pNv21SwsCtx == nullptr) {
            pNv21SwsCtx = sws_getContext(mDisplayWidth, mDisplayHeight, video_fmt,
                                         mDisplayWidth, mDisplayHeight, AV_PIX_FMT_NV21,
                                         SWS_BILINEAR, NULL, NULL, NULL);
        }
    }
    return 0;
}

int VideoPlayer::Play(JNIEnv *env, jobject obj) {
    if (mPlaying) {
        return 1;
    }
    int ret = PreLoad();
    if (ret != 0) {
        return ret;
    }
    mPlaying = true;
    AVPacket packet;
    ANativeWindow_Buffer windowBuffer;
    jbyte *yuvNv21Data = nullptr;
    jbyteArray yuvArray = nullptr;
    int h;
    AVFrame *tmpFrame;

    int yuvLen = mDisplayWidth * mDisplayHeight * 3 / 2;
    size_t y_step = mDisplayWidth * mDisplayHeight * sizeof(uint8_t);

    ANativeWindow_setBuffersGeometry(pNativeWindow, mDisplayWidth, mDisplayHeight,
                                     WINDOW_FORMAT_RGBA_8888);
    if (mNeedNv21Data) {
        yuvNv21Data = new jbyte[yuvLen];
        yuvArray = env->NewByteArray(yuvLen);
    }
    bool error = false;
//    long time, cur;
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
                if (mRotation == ROTATION_0 && !mNeedScaleCat) {
                    //角度是0，并且不需要裁剪
                    tmpFrame = pFrame;
                } else {
                    //旋转并且裁剪
                  if (mRotation == ROTATION_90 || mRotation == ROTATION_270) {
                      if (!mNeedScaleCat) {
                          ret = av_frame_rotate_crop(pFrame, mRotation,
                                                     0, 0, mRotateHeight, mRotateWidth,
                                                     pRotateFrame);
                      } else {
                          ret = av_frame_rotate_crop(pFrame, mRotation,
                                                     mCropTop, mCropLeft, mCropHeight, mCropWidth,
                                                     pRotateFrame);
                      }
                  } else {
                      if (!mNeedScaleCat) {
                          ret = av_frame_rotate_crop(pFrame, mRotation, 0, 0, mRotateWidth,
                                                     mRotateHeight, pRotateFrame);
                      } else {
                          ret = av_frame_rotate_crop(pFrame, mRotation,
                                                     mCropLeft, mCropTop, mCropWidth, mCropHeight,
                                                     pRotateFrame);
                      }
                  }
                    if (ret != 0) {
                        error = true;
                        break;
                    }
                    tmpFrame = pRotateFrame;
                }
                if (mNeedScaleCat) {
                    //缩放
                    ret = av_frame_scale(tmpFrame, pScaleFrame, mScaleWidth, mScaleHeight);
                    if (ret != 0) {
                        error = true;
                        break;
                    }
                    tmpFrame = pScaleFrame;
                }
//                cur = getCurTime();
//                ALOGD("yuv rotate,crop, scale use time %ld", (cur - time));
                //surface绘制
                if (pNativeWindow != nullptr) {
                    sws_scale(pRGBASwsCtx, (uint8_t const *const *) tmpFrame->data,
                              tmpFrame->linesize, 0, mDisplayHeight,
                              pFrameRGBA->data, pFrameRGBA->linesize);
                    if (ANativeWindow_lock(pNativeWindow, &windowBuffer, NULL) >= 0) {
                        uint8_t *dst = (uint8_t *) windowBuffer.bits;
                        int dstStride = windowBuffer.stride * 4;
                        uint8_t *src = (uint8_t *) pFrameRGBA->data[0];
                        size_t src_stride = (size_t) pFrameRGBA->linesize[0];
                        // 由于window的stride和帧的stride不同,因此需要逐行复制
                        for (h = 0; h < mDisplayHeight; h++) {
                            memcpy(dst + h * dstStride, src + h * src_stride, src_stride);
                        }
                        ANativeWindow_unlockAndPost(pNativeWindow);
                    }
                }
                if (mCallBackId != nullptr) {
                    if (mNeedNv21Data) {
                        sws_scale(pNv21SwsCtx, (uint8_t const *const *) tmpFrame->data,
                                  tmpFrame->linesize, 0, mDisplayHeight,
                                  pFrameNv21->data, pFrameNv21->linesize);
                        if (yuvNv21Data != nullptr && yuvArray != nullptr) {
                            memcpy(yuvNv21Data, pFrameNv21->data[0], y_step);//拷贝Y分量
                            memcpy(yuvNv21Data + y_step, pFrameNv21->data[1], y_step / 2);//uv
                            env->SetByteArrayRegion(yuvArray, 0, yuvLen, yuvNv21Data);
                            env->CallVoidMethod(obj, mCallBackId, yuvArray, mDisplayWidth,
                                                mDisplayHeight, mVideoCurDuration,
                                                mVideoAllDuration);
                        }
                    } else {
                        env->CallVoidMethod(obj, mCallBackId, NULL, mDisplayWidth,
                                            mDisplayHeight, mVideoCurDuration, mVideoAllDuration);
                    }
                }
            }
            if (ret < 0 && ret != AVERROR_EOF) {
                error = true;
                break;
            }
        }
        av_packet_unref(&packet);
    }
    ALOGD("end av_read_frame");
    if (yuvArray != nullptr && yuvNv21Data != nullptr) {
        env->ReleaseByteArrayElements(yuvArray, yuvNv21Data, JNI_COMMIT);
    }
    ALOGD("free arr ok");
    if (yuvNv21Data != nullptr) {
        free(yuvNv21Data);
    }
    ALOGD("free data ok");
    mPlaying = false;
    if (error) {
        return ret;
    }
    return 0;
}
