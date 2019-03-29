#include <jni.h>
#include <string>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <syslog.h>

extern "C"{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
};

const char* JNI_CLASS_NAME = "com/max/ffmpegnativewindow/FFmpegTest";

#define TAG "maxD"
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#define LOGE(FORMAT,...) __android_log_print(ANDROID_LOG_ERROR, TAG, FORMAT,##__VA_ARGS__);
#define LOGW(FORMAT,...) __android_log_print(ANDROID_LOG_WARN, TAG, FORMAT,##__VA_ARGS__);
#define LOGI(FORMAT,...) __android_log_print(ANDROID_LOG_INFO, TAG, FORMAT,##__VA_ARGS__);
#define LOGD(FORMAT,...) __android_log_print(ANDROID_LOG_DEBUG, TAG, FORMAT,##__VA_ARGS__);

static int play_status = 0;

unsigned char *getYuv420P(AVFrame *frame, int width, int height) {
    play_status = 1;
    int picSize = height * width;
    int newSize = picSize * 3 / 2;
//申请内存
    unsigned char *buf = new unsigned char[newSize];
//写入数据
    int a = 0, i;
    for (i = 0; i < height; i++) {
        memcpy(buf + a, frame->data[0] + i * frame->linesize[0], width);
        a += width;
    }
    for (i = 0; i < height / 2; i++) {
        memcpy(buf + a, frame->data[1] + i * frame->linesize[1], width / 2);
        a += width / 2;
    }
    for (i = 0; i < height / 2; i++) {
        memcpy(buf + a, frame->data[2] + i * frame->linesize[2], width / 2);
        a += width / 2;
    }
    return buf;
}


jint jni_play(JNIEnv *env, jclass clazz, jobject surface, jstring filePath){

//    const char* file = "/sdcard/test1.mp4";
    const char * pFile = env->GetStringUTFChars(filePath, NULL );
    LOGD("play file %s ", pFile);
    av_register_all();

    AVFormatContext *pFormatCtx = avformat_alloc_context();

    if (avformat_open_input(&pFormatCtx, pFile, NULL, NULL) != 0){
        LOGW("avformat_open error. ");
        env->ReleaseStringUTFChars(filePath, pFile);
        return -1;
    }
    env->ReleaseStringUTFChars(filePath, pFile);
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0){
        LOGW("avformat find stream info failed. ");
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
        LOGE("Didn't find a video stream.");
        return -1; // Didn't find a video stream
    }

    // Get a pointer to the codec context for the video stream
    AVCodecContext * pCodecCtx = pFormatCtx->streams[videoStream]->codec;

    // Find the decoder for the video stream
    AVCodec *pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        LOGE("Codec not found.");
        return -1; // Codec not found
    }

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGE("Could not open codec.");
        return -1; // Could not open codec
    }

    // 获取native window
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);

    // 获取视频宽高
    int videoWidth = pCodecCtx->width;
    int videoHeight = pCodecCtx->height;

    LOGI("video width=%d height=%d ", videoWidth, videoHeight);
    // 设置native window的buffer大小,可自动拉伸
    ANativeWindow_setBuffersGeometry(nativeWindow, videoWidth, videoHeight, WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer windowBuffer;

    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        LOGE("Could not open codec.");
        return -1; // Could not open codec
    }

    // Allocate video frame
    AVFrame *pFrame = av_frame_alloc();

    // 用于渲染
    AVFrame *pFrameRGBA = av_frame_alloc();
    if (pFrameRGBA == NULL || pFrame == NULL) {
        LOGE("Could not allocate video frame.");
        return -1;
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
    LOGE("program use argb. ");
    // Determine required buffer size and allocate buffer
    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGBA,
                                            pCodecCtx->width, pCodecCtx->height, 1);

    uint8_t *buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

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
    AVPacket packet;
    //AV_PIX_FMT_YUV420P=0
    LOGD("av_read_frame start, fmt=%x, play_status=%d", pCodecCtx->pix_fmt, play_status);
    int frameFinished = 0;
    while (av_read_frame(pFormatCtx, &packet) >= 0 && play_status > 0) {
        // Is this a packet from the video stream?
        if (packet.stream_index == videoStream) {
            // Decode video frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
            // 并不是decode一次就可解码出一帧
            if (frameFinished) {
                // 设置缓存区
                ANativeWindow_setBuffersGeometry(nativeWindow, pCodecCtx->width,
                                                 pCodecCtx->height, WINDOW_FORMAT_RGBA_8888);
                // lock native window buffer
                ANativeWindow_lock(nativeWindow, &windowBuffer, 0);

                // 格式转换
                sws_scale(sws_ctx, (uint8_t const *const *) pFrame->data,
                          pFrame->linesize, 0, pCodecCtx->height,
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
    av_free(pFrameRGBA);
    // Free the YUV frame
    av_free(pFrame);
    // Close the codecs
    avcodec_close(pCodecCtx);
    // Close the video file
    avformat_free_context(pFormatCtx);
    return 0;
}

void jni_set_status(JNIEnv *env, jclass clazz, jint status){
    play_status = (int)status;
}

/*
boolean	Z
long	J
byte	B
float	F
char	C
double	D
short	S
类	L全限定类名;
int	I
数组	[元素类型签名
 */
JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *) {
    JNIEnv *env;
    vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
    jclass nativeEngineClass = (jclass) env->NewGlobalRef(env->FindClass(JNI_CLASS_NAME));
    static JNINativeMethod methods[] = {
            {"native_play", "(Landroid/view/Surface;Ljava/lang/String;)I", (void *) jni_play},
            {"native_set_status", "(I)V", (void *) jni_set_status},
    };
    int len = 2;
    if (env->RegisterNatives(nativeEngineClass, methods, len) < 0) {
        return JNI_ERR;
    }
    return JNI_VERSION_1_6;
}


