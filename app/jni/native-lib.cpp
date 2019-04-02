
#include <string>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <syslog.h>
#include "debug.h"
#include "av_rotation.h"

extern "C"{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
};

jint jni_test_play(JNIEnv *env, jclass clazz, jobject surface, jstring filePath){

//    const char* file = "/sdcard/test1.mp4";
    const char * pFile = env->GetStringUTFChars(filePath, NULL );
    ALOGD("play file %s ", pFile);
    av_register_all();

    AVFormatContext *pFormatCtx = avformat_alloc_context();

    if (avformat_open_input(&pFormatCtx, pFile, NULL, NULL) != 0){
        ALOGW("avformat_open error. ");
        env->ReleaseStringUTFChars(filePath, pFile);
        return -1;
    }
    env->ReleaseStringUTFChars(filePath, pFile);
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0){
        ALOGW("avformat find stream info failed. ");
        return -1;
    }

    // Find the first video stream
    int videoStream = -1, i;
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO
            && videoStream < 0) {
            videoStream = i;
        }
    }
    if (videoStream == -1) {
        ALOGE("Didn't find a video stream.");
        return -1; // Didn't find a video stream
    }

    // Get a pointer to the codec context for the video stream
    AVCodecContext * pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    int rotation = av_get_rotation(pFormatCtx->streams[videoStream]);
    int playWidth, playHeight;
    if ((rotation == ROTATION_90 || rotation == ROTATION_270)) {
        playWidth = pCodecCtx->height;
        playHeight = pCodecCtx->width;
    } else {
        playWidth = pCodecCtx->width;
        playHeight = pCodecCtx->height;
    }
    ALOGD("video:rotation=%d, width=%d,height=%d", rotation, pCodecCtx->width, pCodecCtx->height);
    // Find the decoder for the video stream
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        ALOGE("Codec not found.");
        return -1; // Codec not found
    }

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        ALOGE("Could not open codec.");
        return -1; // Could not open codec
    }

    // 获取native window
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);

    // 获取视频宽高
    int videoWidth = pCodecCtx->width;
    int videoHeight = pCodecCtx->height;

    ALOGI("video width=%d height=%d ", videoWidth, videoHeight);
    // 设置native window的buffer大小,可自动拉伸
    ANativeWindow_setBuffersGeometry(nativeWindow, videoWidth, videoHeight, WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer windowBuffer;

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        ALOGE("Could not open codec.");
        return -1; // Could not open codec
    }

    // Allocate video frame
    AVFrame *pFrame = av_frame_alloc();
    AVFrame *pRotationFrame = av_frame_alloc();

    uint8_t *rbuffer = (uint8_t *) av_malloc(
            av_image_get_buffer_size(AV_PIX_FMT_YUV420P, videoWidth, videoHeight, 1) *
            sizeof(uint8_t));

    av_image_fill_arrays(pRotationFrame->data, pRotationFrame->linesize,
                         rbuffer, AV_PIX_FMT_YUV420P,
                         videoWidth, videoHeight, 1);
    // 用于渲染
    AVFrame *pFrameRGBA = av_frame_alloc();
    // Determine required buffer size and allocate buffer
    uint8_t *buffer = (uint8_t *) av_malloc(av_image_get_buffer_size(AV_PIX_FMT_RGBA,
                                                                     pCodecCtx->width,
                                                                     pCodecCtx->height, 1) *
                                            sizeof(uint8_t));

    av_image_fill_arrays(pFrameRGBA->data, pFrameRGBA->linesize,
                         buffer, AV_PIX_FMT_RGBA,
                         pCodecCtx->width, pCodecCtx->height, 1);

    // 由于解码出来的帧格式不是RGBA的,在渲染之前需要进行格式转换
    struct SwsContext *sws_ctx = sws_getContext(pCodecCtx->width,
                                                pCodecCtx->height,
                                                pCodecCtx->pix_fmt,
                                                pCodecCtx->width,
                                                pCodecCtx->height,
                                                AV_PIX_FMT_RGBA,
                                                SWS_BILINEAR,
                                                NULL,
                                                NULL,
                                                NULL);

    AVFrame *tmpFrame;
    AVPacket packet;
    //AV_PIX_FMT_YUV420P=0
    ALOGD("av_read_frame start, fmt=%x", pCodecCtx->pix_fmt);
    int frameFinished = 0;

    int srcHeight = 0;
    // 设置缓存区
    ANativeWindow_setBuffersGeometry(nativeWindow, playWidth, playHeight, WINDOW_FORMAT_RGBA_8888);
    while (av_read_frame(pFormatCtx, &packet) >= 0) {
        // Is this a packet from the video stream?
        if (packet.stream_index == videoStream) {
            // Decode video frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
            // 并不是decode一次就可解码出一帧
            if (frameFinished) {
                if (rotation == ROTATION_90) {
                    av_frame_rotate_90(pFrame, pRotationFrame);
                    tmpFrame = pRotationFrame;
                    srcHeight = videoWidth;
//                } else if (rotation == ROTATION_180) {
//                    av_frame_rotate_180(pFrame, pRotationFrame);
//                    tmpFrame = pRotationFrame;
//                    srcHeight = videoHeight;
//                } else if (rotation == ROTATION_270) {
//                    av_frame_rotate_270(pFrame, pRotationFrame);
//                    tmpFrame = pRotationFrame;
//                    srcHeight = videoWidth;
                } else {
                    tmpFrame = pFrame;
                    srcHeight = videoHeight;
                }

                // lock native window buffer
                ANativeWindow_lock(nativeWindow, &windowBuffer, NULL);

                // 格式转换
                sws_scale(sws_ctx, (uint8_t const *const *) tmpFrame->data,
                          tmpFrame->linesize, 0, srcHeight,
                          pFrameRGBA->data, pFrameRGBA->linesize);

                // 获取stride
                uint8_t *dst = (uint8_t *) windowBuffer.bits;
                int dstStride = windowBuffer.stride * 4;
                uint8_t *src = (uint8_t *) (pFrameRGBA->data[0]);
                size_t srcStride = (size_t) pFrameRGBA->linesize[0];

                // 由于window的stride和帧的stride不同,因此需要逐行复制
                int h;
                for (h = 0; h < videoHeight; h++) {
                    memcpy(dst + h * dstStride, src + h * srcStride, srcStride);
                }

                ANativeWindow_unlockAndPost(nativeWindow);
            }

        }
        av_packet_unref(&packet);
    }
    //free
    ANativeWindow_release(nativeWindow);
    av_free(buffer);
//    av_free(rbuffer);
    av_free(pFrameRGBA);
    // Free the YUV frame
    av_free(pFrame);
    // Close the codecs
    avcodec_close(pCodecCtx);
    // Close the video file
    avformat_free_context(pFormatCtx);
    return 0;
}


