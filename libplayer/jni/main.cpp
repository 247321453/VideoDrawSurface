//
// Created by user on 2019/4/2.
//
#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "debug.h"
#include "video_player.h"

#include "yuv_util.h"
#include "yuv_jni.h"
#include "yuv_draw.h"
#include "util.h"

#define JNI_CLASS_NAME "net/kk/ffmpeg/VideoPlayer"
#define JNI_YUV_UTIL_CLASS_NAME "net/kk/ffmpeg/YuvUtil"

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

void
jni_player_set_size(JNIEnv *env, jobject obj, jlong ptr, jint width, jint height,
                    jboolean stretch, jint rotation) {
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        player->Release(true);
        player->SetSize(width, height, stretch, rotation);
    }
}

jint jni_player_get_width(JNIEnv *env, jobject obj, jlong ptr) {
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        return player->GetVideoInfo().src_width;
    }
    return -1;
}

jint jni_player_get_height(JNIEnv *env, jobject obj, jlong ptr) {
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        return player->GetVideoInfo().src_height;
    }
    return -1;
}

jint jni_player_get_rotate(JNIEnv *env, jobject obj, jlong ptr) {
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        return player->GetVideoInfo().src_rotation;
    }
    return -1;
}

void
jni_player_set_surface(JNIEnv *env, jobject obj, jlong ptr, jobject surface) {
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
        player->SetSurface(nativeWindow);
    }
}

void
jni_player_set_callback(JNIEnv *env, jobject obj, jlong ptr, jboolean callback) {
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        jclass clazz = env->GetObjectClass(obj);
        jmethodID yuv_callback = env->GetMethodID(clazz, "onFrameCallBack", "([BIIDD)V");
        jmethodID jpeg_callback = env->GetMethodID(clazz, "onImageCallBack", "([BII)V");
        if (yuv_callback != NULL) {
            player->SetCallBack(yuv_callback, jpeg_callback, callback);
        } else {
            ALOGW("not found callback onFrameCallBack([BIIDD)V");
        }
    }
}

void jni_set_data_source(JNIEnv *env, jobject obj, jlong ptr, jstring path) {
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        const char *_filename = env->GetStringUTFChars(path, NULL);
        int len = env->GetStringLength(path);
        char *filename = new char[len + 1];
        strcpy(filename, _filename);
        filename[len] = '\0';
        ALOGD("set datasource %d=%s", len, filename);
        env->ReleaseStringUTFChars(path, _filename);
        player->SetDataSource(filename);
    }
}

jint jni_player_preload(JNIEnv *env, jobject obj, jlong ptr) {
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        return player->PreLoad();
    }
    return -1;
}

jint jni_player_play(JNIEnv *env, jobject obj, jlong ptr) {
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        return player->Play(env, obj);
    }
    return -1;
}

jint jni_player_get_status(JNIEnv *env, jobject obj, jlong ptr) {
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        if (player->IsPlaying()) {
            return 1;
        }
        return 0;
    }
    return -1;
}

void jni_player_stop(JNIEnv *env, jobject obj, jlong ptr) {
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        player->Stop();
    }
}

jlong jni_create_player(JNIEnv *env, jclass) {
    kk::VideoPlayer *player = new kk::VideoPlayer;
    return (jlong) player;
}

void jni_player_close(JNIEnv *env, jobject obj, jlong ptr) {
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        player->Close();
        free(player);
    }
}

void jni_player_release(JNIEnv *env, jobject obj, jlong ptr) {
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        player->Release(false);
    }
}

jint jni_player_seek(JNIEnv *env, jobject obj, jlong ptr, jdouble time) {
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        return player->Seek(time);
    }
    return -1;
}

jint
jni_player_take_image(JNIEnv *env, jobject obj, jlong ptr, jint width, jint height, jint rotation,
                      jboolean mirror) {
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        return player->TakeImage(env, obj, width, height, rotation, mirror);
    }
    return -1;
}

double jni_player_get_play_time(JNIEnv *env, jobject, jlong ptr) {
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        return player->GetVideoDuration();
    }
    return 0;
}

double jni_player_get_video_time(JNIEnv *env, jobject, jlong ptr) {
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        return player->GetVideoDuration();
    }
    return 0;
}


jint jni_player_get_last_frame(JNIEnv *env, jobject, jlong ptr, jbyteArray arr) {
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        uint8_t *data = player->GetVideoLastFrame();
        int len = player->GetVideoLastLength();
        env->SetByteArrayRegion(arr, 0, len, (jbyte *) data);
        return 0;
    }
    return 0;
}

void jni_ffmpeg_init(JNIEnv *env, jclass) {
    av_register_all();
}

jint jni_i420_rotate_crop_ex(JNIEnv *env, jclass,
                             jbyteArray src_i420_data,
                             jint src_width, jint src_height, jint src_rotation,
                             jbyteArray dst_i420_data,
                             jint dst_width, jint dst_height, jint dst_rotation,
                             jboolean stretch) {
    jbyte *src = env->GetByteArrayElements(src_i420_data, NULL);
    jbyte *dst = env->GetByteArrayElements(dst_i420_data, NULL);
    kk::VideoInfo info;
    info.src_width = src_width;
    info.src_height = src_height;
    info.src_rotation = src_rotation;
    kk::initVideoSize(&info, dst_width, dst_height, dst_rotation, stretch);

    int crop_x, crop_y, crop_w, crop_h;
    if (info.need_swap_size) {
        if (info.need_scale) {
            crop_x = info.crop_y;
            crop_y = info.crop_x;
            crop_w = info.crop_height;
            crop_h = info.crop_width;
        } else {
            crop_x = 0;
            crop_y = 0;
            crop_w = info.rotate_height;
            crop_h = info.rotate_width;
        }
    } else {
        if (info.need_scale) {
            crop_x = info.crop_x;
            crop_y = info.crop_y;
            crop_w = info.crop_width;
            crop_h = info.crop_height;
        } else {
            crop_x = 0;
            crop_y = 0;
            crop_w = info.rotate_width;
            crop_h = info.rotate_height;
        }
    }
    int ret;
    if (info.need_scale) {
        uint8_t *r_data = new uint8_t[crop_w * crop_h * 3 / 2];
        ret = i420_rotate_crop((uint8_t *) src, src_width, src_height, info.display_rotation,
                               crop_x, crop_y, crop_w, crop_h, r_data);
        if (ret == 0) {
            ret = i420_scale(r_data, crop_x, crop_h, (uint8_t *) dst, info.display_width,
                             info.display_height, 3);
        }
        free(r_data);
    } else {
        ret = i420_rotate_crop((uint8_t *) src, src_width, src_height, info.display_rotation,
                               crop_x, crop_y, crop_w, crop_h, (uint8_t *) dst);
    }
    env->ReleaseByteArrayElements(src_i420_data, src, 0);
    env->ReleaseByteArrayElements(dst_i420_data, dst, 0);
    return ret;
}


JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *) {
    JNIEnv *env;
    vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
    jclass video_player = (jclass) env->NewGlobalRef(env->FindClass(JNI_CLASS_NAME));
    static JNINativeMethod methods[] = {
            {"native_create",          "()J",                        (void *) jni_create_player},
            {"native_set_surface",     "(JLandroid/view/Surface;)V", (void *) jni_player_set_surface},
            {"native_set_callback",    "(JZ)V",                      (void *) jni_player_set_callback},
            {"native_set_size",        "(JIIZI)V",                   (void *) jni_player_set_size},
            {"native_get_width",       "(J)I",                       (void *) jni_player_get_width},
            {"native_get_height",      "(J)I",                       (void *) jni_player_get_height},
            {"native_get_rotate",      "(J)I",                       (void *) jni_player_get_rotate},
            {"native_set_data_source", "(JLjava/lang/String;)V",     (void *) jni_set_data_source},
            {"native_play",            "(J)I",                       (void *) jni_player_play},
            {"native_preload",         "(J)I",                       (void *) jni_player_preload},
            {"native_stop",            "(J)V",                       (void *) jni_player_stop},
            {"native_close",           "(J)V",                       (void *) jni_player_close},
            {"native_release",         "(J)V",                       (void *) jni_player_release},
            {"native_seek",            "(JD)I",                      (void *) jni_player_seek},
            {"native_take_image",      "(JIIIZ)I",                   (void *) jni_player_take_image},
            {"native_get_cur_time",    "(J)D",                       (void *) jni_player_get_play_time},
            {"native_get_all_time",    "(J)D",                       (void *) jni_player_get_video_time},
            {"native_get_status",      "(J)I",                       (void *) jni_player_get_status},
            {"native_init_ffmpeg",     "()V",                        (void *) jni_ffmpeg_init},
            {"native_get_last_frame",  "(J[B)I",                     (void *) jni_player_get_last_frame},
            //  {"native_test_play", "(Landroid/view/Surface;Ljava/lang/String;)I", (void *) jni_test_play},
    };
    if (env->RegisterNatives(video_player, methods, 20) < 0) {
        return JNI_ERR;
    }
    jclass yuv_util = (jclass) env->NewGlobalRef(env->FindClass(JNI_YUV_UTIL_CLASS_NAME));
    static JNINativeMethod yuv_methods[] = {
            {"i420ToArgb",         "([BII[B)I",                     (void *) jni_i420_to_argb},
            {"nv21ToArgb",         "([BII[B)I",                     (void *) jni_nv21_to_argb},
            {"argbToI420",         "([BII[B)I",                     (void *) jni_argb_to_i420},

            {"argbToNv21",         "([BII[B)I",                     (void *) jni_argb_to_nv21},
            {"i420ToNv21",         "([BII[B)I",                     (void *) jni_i420_to_nv21},
            {"i420Mirror",         "([BII[B)I",                     (void *) jni_i420_mirror},

            {"i420RotateWithCrop", "([BIII[BIIII)I",                (void *) jni_i420_rotate_crop},
            {"i420RotateWithCropEx", "([BIII[BIIIZ)I",                (void *) jni_i420_rotate_crop_ex},
            {"nv21ToI420",         "([BII[B)I",                     (void *) jni_nv21_to_i420},
            {"i420Scale",          "([BII[BIII)I",                  (void *) jni_i420_scale},

            {"rgbaToI420",         "([BII[B)I",                     (void *) jni_rgba_to_i420},
            {"rgbaToNv21",         "([BII[B)I",                     (void *) jni_rgba_to_nv21},

            {"rgbaDrawSurface",    "(Landroid/view/Surface;[BII)I", (void *) jni_rgba_draw_surface},

            {"i420DrawSurface",    "(Landroid/view/Surface;[BII)I", (void *) jni_i420_draw_surface},
            {"nv21DrawSurface",    "(Landroid/view/Surface;[BII)I", (void *) jni_nv21_draw_surface},
    };

    if (env->RegisterNatives(yuv_util, yuv_methods, 14) < 0) {
        return JNI_ERR;
    }
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
}