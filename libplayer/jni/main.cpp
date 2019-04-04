//
// Created by user on 2019/4/2.
//
#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include "debug.h"
#include "video_player.h"

#define JNI_CLASS_NAME "net/kk/ffmpeg/VideoPlayer"

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
jlong jni_create_player(JNIEnv *env, jclass) {
    kk::VideoPlayer *player = new kk::VideoPlayer;
    return (jlong) player;
}

void
jni_player_set_size(JNIEnv *env, jobject obj, jlong ptr, jint width, jint height,
                    jboolean stretch, jint rotation) {
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        player->Release(true);
        player->SetSize(width, height, stretch, rotation);
    }
}
jint jni_player_get_width(JNIEnv *env, jobject obj, jlong ptr){
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        return player->GetVideoWidth();
    }
    return -1;
}
jint jni_player_get_height(JNIEnv *env, jobject obj, jlong ptr){
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        return player->GetVideoHeight();
    }
    return -1;
}
jint jni_player_get_rotate(JNIEnv *env, jobject obj, jlong ptr){
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        return player->GetVideoRotation();
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
        jmethodID javaCallback = env->GetMethodID(clazz, "onFrameCallBack", "([BIIDD)V");
        if (javaCallback != NULL) {
            player->SetCallBack(javaCallback, callback);
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

double jni_player_get_play_time(JNIEnv *env, jobject obj, jlong ptr) {
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        return player->GetVideoDuration();
    }
    return 0;
}

double jni_player_get_video_time(JNIEnv *env, jobject obj, jlong ptr) {
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        return player->GetVideoDuration();
    }
    return 0;
}

void jni_ffmpeg_init(JNIEnv *env, jclass) {
    av_register_all();
}


JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *) {
    JNIEnv *env;
    vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
    jclass nativeEngineClass = (jclass) env->NewGlobalRef(env->FindClass(JNI_CLASS_NAME));
    static JNINativeMethod methods[] = {
            {"native_create",         "()J",                         (void *) jni_create_player},
            {"native_set_surface",    "(JLandroid/view/Surface;)V",  (void *) jni_player_set_surface},
            {"native_set_callback",   "(JZ)V",                       (void *) jni_player_set_callback},
            {"native_set_size",       "(JIIZI)V",                      (void *) jni_player_set_size},
            {"native_get_width",       "(J)I",                      (void *) jni_player_get_width},
            {"native_get_height",       "(J)I",                      (void *) jni_player_get_height},
            {"native_get_rotate",       "(J)I",                      (void *) jni_player_get_rotate},
            {"native_set_data_source", "(JLjava/lang/String;)V",      (void *) jni_set_data_source},
            {"native_play",           "(J)I",                        (void *) jni_player_play},
            {"native_preload",        "(J)I",                        (void *) jni_player_preload},
            {"native_stop",           "(J)V",                        (void *) jni_player_stop},
            {"native_close",          "(J)V",                        (void *) jni_player_close},
            {"native_release",        "(J)V",                        (void *) jni_player_release},
            {"native_seek",           "(JD)I",                       (void *) jni_player_seek},
            {"native_get_cur_time",   "(J)D",                        (void *) jni_player_get_play_time},
            {"native_get_all_time",   "(J)D",                        (void *) jni_player_get_video_time},
            {"native_get_status",     "(J)I",                        (void *) jni_player_get_status},
            {"native_init_ffmpeg",    "()V",                         (void *) jni_ffmpeg_init},
            //  {"native_test_play", "(Landroid/view/Surface;Ljava/lang/String;)I", (void *) jni_test_play},
    };
    if (env->RegisterNatives(nativeEngineClass, methods, 18) < 0) {
        return JNI_ERR;
    }
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
}