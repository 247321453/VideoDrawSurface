//
// Created by user on 2019/4/2.
//

#include <android/native_window_jni.h>
#include <unistd.h>
#include <yuv_util.h>
#include "video_player.h"
#include "util.h"
#include "debug.h"

using namespace kk;

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
    }
    //角度，宽高
    Info.src_rotation = av_get_rotation(steam);
    ALOGD("src_rotation=%d", Info.src_rotation);
    pCodecCtx = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(pCodecCtx, steam->codecpar);
    Info.src_width = pCodecCtx->width;
    Info.src_height = pCodecCtx->height;
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        return -5;
    }
    //软解
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        return -6;
    }
    mNeedToI420 = (pCodecCtx->pix_fmt != AV_PIX_FMT_YUV420P);
    mPreLoad = true;
    ALOGD("PreLoad::ok");
    return 0;
}

void VideoPlayer::Close() {
    mPlaying = false;
    mYuvCallBackId = nullptr;
    mJpegCallBackId = nullptr;
    if (pNativeWindow != nullptr) {
        ANativeWindow_release(pNativeWindow);
        pNativeWindow = nullptr;
    }
    ALOGD("release by Close");
    Release(false);
}

void VideoPlayer::Release(bool resize) {
    //pI420SwsCtx
    if (pI420SwsCtx != nullptr) {
        av_free(pI420SwsCtx);
        pI420SwsCtx = nullptr;
    }
    if (pRGBASwsCtx != nullptr) {
        av_free(pRGBASwsCtx);
        pRGBASwsCtx = nullptr;
    }
    if (pNv21SwsCtx != nullptr) {
        av_free(pNv21SwsCtx);
        pNv21SwsCtx = nullptr;
    }

    if (pTakeYuvBuf != nullptr) {
        free(pTakeYuvBuf);
        pVideoYuvLen = -1;
        pTakeYuvBuf = nullptr;
    }
    if (pScaleBuf != nullptr) {
        av_free(pScaleBuf);
        pScaleBuf = nullptr;
    }
    if (pScaleFrame != nullptr) {
        av_free(pScaleFrame);
        pScaleFrame = nullptr;
    }

    if (pPreviewBuf != nullptr) {
        av_free(pPreviewBuf);
        pPreviewBuf = nullptr;
    }
    if (pPreviewFrame != nullptr) {
        av_free(pPreviewFrame);
        pPreviewFrame = nullptr;
    }

    if (pI420Buf != nullptr) {
        av_free(pI420Buf);
        pI420Buf = nullptr;
    }
    if (pFrameI420 != nullptr) {
        av_free(pFrameI420);
        pFrameI420 = nullptr;
    }


    if (pRotateCropBuf != nullptr) {
        av_free(pRotateCropBuf);
        pRotateCropBuf = nullptr;
    }
    if (pRotateCropFrame != nullptr) {
        av_free(pRotateCropFrame);
        pRotateCropFrame = nullptr;
    }
    if (pRgbBuf != nullptr) {
        av_free(pRgbBuf);
        pRgbBuf = nullptr;
    }
    if (pNv21Buf != nullptr) {
        av_free(pNv21Buf);
        pNv21Buf = nullptr;
    }
    if (pFrameRGB != nullptr) {
        av_free(pFrameRGB);
        pFrameRGB = nullptr;
    }
    if (pFrameNv21 != nullptr) {
        av_free(pFrameNv21);
        pFrameNv21 = nullptr;
    }
    if (pFrame != nullptr) {
        av_free(pFrame);
        pFrame = nullptr;
    }
    if (!resize) {
        if (pCodecCtx != nullptr) {
            avcodec_close(pCodecCtx);
            pCodecCtx = nullptr;
        }
        if (pFormatCtx != nullptr) {
            avformat_free_context(pFormatCtx);
            pFormatCtx = nullptr;
        }
    }
}

int VideoPlayer::Seek(double ms) {
    if (mPreLoad && pFormatCtx != NULL && mVideoStream != -1) {
        //ms * AV_TIME_BASE / 1000
        int ret = av_seek_frame(pFormatCtx, mVideoStream, static_cast<int64_t>(ms * AV_TIME_BASE),
                                AVSEEK_FLAG_BACKWARD);

        if (ret == 0) {
            avcodec_flush_buffers(pCodecCtx);
        }
        return ret;
    }
    return -1;
}

int VideoPlayer::initData() {
    //原始
    if (pFrame == nullptr) {
        pFrame = av_frame_alloc();
    }
    if (mNeedToI420) {
        if (pFrameI420 == nullptr) {
            pFrameI420 = av_frame_alloc();
            pI420Buf = initFrame(pFrameI420, AV_PIX_FMT_YUV420P, Info.src_width, Info.src_height);
            ALOGD("init j420p frame %dx%d", Info.src_width, Info.src_height);
        }
        if (pI420SwsCtx == nullptr) {
            pI420SwsCtx = sws_getContext(Info.src_width, Info.src_height, pCodecCtx->pix_fmt,
                                         Info.src_width, Info.src_height, AV_PIX_FMT_YUV420P,
                                         SWS_BILINEAR, NULL, NULL, NULL);
        }
    }
    AVPixelFormat video_fmt = AV_PIX_FMT_YUV420P;
    //存放原始数据
    if (pTakeYuvBuf == nullptr) {
        pVideoYuvLen = Info.src_width * Info.src_height * 3 / 2;
        pTakeYuvBuf = new uint8_t[pVideoYuvLen];
    }
    if (Info.display_rotation != ROTATION_0 || Info.need_scale) {
        if (pRotateCropFrame == nullptr) {
            pRotateCropFrame = av_frame_alloc();
            pRotateCropBuf = initFrame(pRotateCropFrame, video_fmt, Info.rotate_width,
                                       Info.rotate_height);
            ALOGD("init rotate frame %dx%d", Info.rotate_width, Info.rotate_height);
        }
    }
    if (Info.need_scale) {
        if (pScaleFrame == nullptr) {
            pScaleFrame = av_frame_alloc();
            pScaleBuf = initFrame(pScaleFrame, video_fmt, Info.scale_width, Info.scale_height);
            ALOGD("init scale frame %dx%d", Info.scale_width, Info.scale_height);
        }
    }

    if (pNativeWindow != nullptr) {
        AVPixelFormat rgb_fmt = mRGB565Mode ? AV_PIX_FMT_RGB565 : AV_PIX_FMT_RGBA;
        int pre_w = Info.display_width;
        int pre_h = Info.display_height;
        if (mFixRotation != ROTATION_0) {
            if (mFixRotation != ROTATION_180) {
                pre_h = Info.display_width;
                pre_w = Info.display_height;
            }
            if (pPreviewFrame == nullptr) {
                pPreviewFrame = av_frame_alloc();
                pPreviewBuf = initFrame(pPreviewFrame, video_fmt, pre_w, pre_h);
                ALOGD("init rgba pre frame r=%d, size=%dx%d", mFixRotation, pre_w, pre_h);
            }
        }
        if (pFrameRGB == nullptr) {
            pFrameRGB = av_frame_alloc();
            pRgbBuf = initFrame(pFrameRGB, rgb_fmt, pre_w, pre_h);
            ALOGD("init rgba frame %dx%d, rgb565=%d", pre_w, pre_h, mRGB565Mode);
        }
        if (pRGBASwsCtx == nullptr) {
            pRGBASwsCtx = sws_getContext(pre_w, pre_h, video_fmt, pre_w, pre_h, rgb_fmt,
                                         SWS_FAST_BILINEAR, NULL, NULL, NULL);
        }
    }

    if (mNeedNv21Data) {
        if (pFrameNv21 == nullptr) {
            pFrameNv21 = av_frame_alloc();
            pNv21Buf = initFrame(pFrameNv21, AV_PIX_FMT_NV21, Info.display_width,
                                 Info.display_height);
            ALOGD("init nv21 frame %dx%d", Info.display_width, Info.display_height);
        }
        if (pNv21SwsCtx == nullptr) {
            pNv21SwsCtx = sws_getContext(Info.display_width, Info.display_height, video_fmt,
                                         Info.display_width, Info.display_height, AV_PIX_FMT_NV21,
                                         SWS_FAST_BILINEAR, NULL, NULL, NULL);
        }
    }
    return 0;
}

int VideoPlayer::Play(JNIEnv *env, jobject obj) {
    if (mPlaying) {
        return 1;
    }
    mPlaying = true;
    int ret = PreLoad();
    if (ret != 0) {
        return ret;
    }
    InitSizeInfo(&Info, mPreviewWidth, mPreviewHeight, mPreRotation, mStretchMode, 0);
    ret = initData();
    if (ret != 0) {
        return ret;
    }
    AVPacket packet;
    ANativeWindow_Buffer windowBuffer;
    jbyte *yuvNv21Data = nullptr;
    jbyteArray yuvArray = nullptr;
    int h;
    AVFrame *tmpFrame;

    int yuvLen = Info.display_width * Info.display_height * 3 / 2;
    size_t y_step = Info.display_width * Info.display_height * sizeof(uint8_t);

    if (mNeedNv21Data) {
        yuvNv21Data = new jbyte[yuvLen];
        yuvArray = env->NewByteArray(yuvLen);
    }
    bool error = false;
   // long time, cur;
    int crop_x = Info.crop_x, crop_y = Info.crop_y, crop_w = Info.crop_width, crop_h = Info.crop_height;
    ALOGD("src=%dx%d, dst=%dx%d crop %d,%d %dx%d",
          Info.src_width, Info.src_height,
          Info.display_width, Info.display_height,
          crop_x, crop_y, crop_w, crop_h);
    int surface_width = Info.display_width;
    int surface_height = Info.display_height;
    if (pNativeWindow == nullptr) {
        ret = 0;
    } else {
        if (mFixRotation == ROTATION_90 || mFixRotation == ROTATION_270) {
            surface_height = Info.display_width;
            surface_width = Info.display_height;
        }
        ret = ANativeWindow_setBuffersGeometry(pNativeWindow, surface_width, surface_height,
                                               mRGB565Mode ? WINDOW_FORMAT_RGB_565
                                                           : WINDOW_FORMAT_RGBA_8888);
    }
    if (ret != 0) {
        ALOGD("ANativeWindow_setBuffersGeometry error %d", ret);
    } else {
        ALOGD("start av_read_frame");
        while (mPlaying && av_read_frame(pFormatCtx, &packet) >= 0) {
            //use the parser to split the data into frames
            //微秒
            if (packet.stream_index == mVideoStream) {
                ret = avcodec_send_packet(pCodecCtx, &packet);

                if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                    error = true;
                    break;
                }

                while (mPlaying && avcodec_receive_frame(pCodecCtx, pFrame) == 0) {
                    av_frame_to_i420(pFrame, pTakeYuvBuf);
                    mVideoCurDuration = pFrame->pts * av_q2d(pTimeBase);
                 //   time = getCurTime();
                    if (mNeedToI420) {
                        sws_scale(pI420SwsCtx, (uint8_t const *const *) pFrame->data,
                                  pFrame->linesize, 0, Info.src_height,
                                  pFrameI420->data, pFrameI420->linesize);
                        tmpFrame = pFrameI420;
                        //cur = getCurTime();
                       // ALOGD("i420 time %ld", (cur - time));
                       // time = cur;
                    } else {
                        tmpFrame = pFrame;
                    }

                    if (Info.display_rotation == ROTATION_0 && !Info.need_crop) {
                        //角度是0，并且不需要裁剪
                    } else {
                        //旋转并且裁剪
                        ret = av_frame_rotate_crop(tmpFrame, Info.display_rotation,
                                                   crop_x, crop_y, crop_w, crop_h,
                                                   pRotateCropFrame);
                        if (ret != 0) {
                            error = true;
                            ALOGE("av_frame_rotate_crop error %d", ret);
                            break;
                        }
                        tmpFrame = pRotateCropFrame;
                       // cur = getCurTime();
                      //  ALOGD("av_frame_rotate_crop time %ld", (cur-time));
                      //  time = cur;
                    }
                    if (Info.need_scale) {
                        //缩放
                        ret = av_frame_scale(tmpFrame, pScaleFrame, Info.scale_width,
                                             Info.scale_height);
                        if (ret != 0) {
                            error = true;
                            ALOGE("av_frame_scale error %d", ret);
                            break;
                        }
                        tmpFrame = pScaleFrame;
                       // cur = getCurTime();
                       // ALOGD("scale time %ld", (cur-time));
                       // time = cur;
                    }
//                cur = getCurTime();
//                ALOGD("yuv rotate,crop, scale use time %ld", (cur - time));
                    if (mYuvCallBackId != nullptr) {
                        if (mNeedNv21Data) {
                            sws_scale(pNv21SwsCtx, (uint8_t const *const *) tmpFrame->data,
                                      tmpFrame->linesize, 0, Info.display_height,
                                      pFrameNv21->data, pFrameNv21->linesize);
                            if (yuvNv21Data != nullptr && yuvArray != nullptr) {
                                memcpy(yuvNv21Data, pFrameNv21->data[0], y_step);//拷贝Y分量
                                memcpy(yuvNv21Data + y_step, pFrameNv21->data[1], y_step / 2);//uv
                                env->SetByteArrayRegion(yuvArray, 0, yuvLen, yuvNv21Data);
                                env->CallVoidMethod(obj, mYuvCallBackId, yuvArray,
                                                    Info.display_width,
                                                    Info.display_height, mVideoCurDuration,
                                                    mVideoAllDuration);
                            }
                        } else {
                            env->CallVoidMethod(obj, mYuvCallBackId, NULL, Info.display_width,
                                                Info.display_height, mVideoCurDuration,
                                                mVideoAllDuration);
                        }
                       // cur = getCurTime();
                      //  ALOGD("callback nv21 %ld", (cur-time));
                      //  time = cur;
                    }
                    //surface绘制
                    if (pNativeWindow != nullptr) {
                        if (mFixRotation != ROTATION_0) {
                            ret = av_frame_rotate(tmpFrame, mFixRotation, pPreviewFrame);
                            if (ret != 0) {
                                error = true;
                                ALOGE("av_frame_rotate error %d", ret);
                                break;
                            }
                            tmpFrame = pPreviewFrame;
                           // cur = getCurTime();
                           // ALOGD("i420 rotate %ld", (cur-time));
                           // time = cur;
                        }
                        sws_scale(pRGBASwsCtx, (uint8_t const *const *) tmpFrame->data,
                                  tmpFrame->linesize, 0, surface_height,
                                  pFrameRGB->data, pFrameRGB->linesize);
                       // cur = getCurTime();
                       // ALOGD("rgba time %ld, h=%d, linesize=%d",
                      //          (cur-time), surface_height, pFrameRGB->linesize[0]);
                      //  time = cur;
                        if (ANativeWindow_lock(pNativeWindow, &windowBuffer, NULL) >= 0) {
                            uint8_t *dst = (uint8_t *) windowBuffer.bits;
                            int dst_stride;
                            uint8_t *src = pFrameRGB->data[0];
                            size_t src_stride = (size_t) pFrameRGB->linesize[0];
                            if (windowBuffer.format == WINDOW_FORMAT_RGB_565) {
                                dst_stride = windowBuffer.stride * 2;
                            } else {
                                dst_stride = windowBuffer.stride * 4;
                            }
                           // ALOGD("draw size=%dx%d, linesize=%d, s=%d", surface_width,
                           //       surface_height, pFrameRGB->linesize[0], dst_stride);
                            // 由于window的stride和帧的stride不同,因此需要逐行复制
                            for (h = 0; h < surface_height; h++) {
                                memcpy(dst + h * dst_stride, src + h * src_stride, src_stride);
                            }
                            ANativeWindow_unlockAndPost(pNativeWindow);
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
    }
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


int VideoPlayer::TakeImage(JNIEnv *env, jobject obj, int dst_width, int dst_height,
                           int dst_rotation, bool mirror) {
    if (pTakeYuvBuf != nullptr && mJpegCallBackId != nullptr) {
        //处理i420得到jpeg数据
        uint8_t *i420 = pTakeYuvBuf;
        int video_width = Info.src_width;
        int video_height = Info.src_height;
        int video_rotation = Info.src_rotation;

        if (dst_rotation < 0) {
            dst_rotation = 0;
            video_rotation = 0;
        }
        SizeInfo info;
        info.src_width = video_width;
        info.src_height = video_height;
        info.src_rotation = video_rotation;
        InitSizeInfo(&info, dst_width, dst_height, dst_rotation % 4, false, 0);

        int ret = 0;
        uint8_t *tmpData;
        uint8_t *cropData = nullptr;
        uint8_t *scaleData = nullptr;
        int w, h;
        if (info.display_rotation == ROTATION_0 && !info.need_crop) {
            //角度是0，并且不需要裁剪
            tmpData = i420;
            w = video_width;
            h = video_height;
        } else {
            //旋转并且裁剪
            int crop_x = info.crop_x, crop_y = info.crop_y, crop_w = info.crop_width, crop_h = info.crop_height;
            if (info.display_rotation == ROTATION_90 || info.display_rotation == ROTATION_270) {
                w = crop_h;
                h = crop_w;
            } else {
                w = crop_w;
                h = crop_h;
            }

            cropData = new uint8_t[crop_w * crop_h * 3 / 2];
            ret = i420_rotate_crop(i420, video_width, video_height, info.display_rotation,
                                   crop_x, crop_y, crop_w, crop_h, cropData);
            tmpData = cropData;
        }
        if (info.need_scale) {
            //缩放
            scaleData = new uint8_t[dst_width * dst_height * 3 / 2];
            ret = i420_scale(tmpData, w, h, scaleData, dst_width, dst_height, 3);
            w = dst_width;
            h = dst_height;
            tmpData = scaleData;
        }
        uint8_t *mirrorData = nullptr;
        if (ret == 0) {
            int yuvLen = w * h * 3 / 2;
            jbyte *yuvNv21Data = new jbyte[yuvLen];
            if (mirror) {
                mirrorData = new uint8_t[yuvLen];
                i420_mirror(tmpData, w, h, mirrorData);
                tmpData = mirrorData;
            }
            i420_to_nv21(tmpData, w, h, (uint8_t *) yuvNv21Data);
            jbyteArray yuvArray = env->NewByteArray(yuvLen);
            env->SetByteArrayRegion(yuvArray, 0, yuvLen, (jbyte *) yuvNv21Data);
            env->CallVoidMethod(obj, mJpegCallBackId, yuvArray, w, h);
            env->ReleaseByteArrayElements(yuvArray, yuvNv21Data, JNI_COMMIT);
            free(yuvNv21Data);
        } else {
            env->CallVoidMethod(obj, mJpegCallBackId, NULL, ret, h);
        }
        if (mirrorData != nullptr) {
            free(mirrorData);
        }
        if (scaleData != nullptr) {
            free(scaleData);
        }
        if (cropData != nullptr) {
            free(cropData);
        }
        return 0;
    }
    return -1;
}
