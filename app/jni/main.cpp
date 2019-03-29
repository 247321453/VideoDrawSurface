//
// Created by user on 2019/4/2.
//
#include "main.h"

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
jni_player_set_callback(JNIEnv *env, jobject obj, jlong ptr, jobject surface, jboolean callback) {
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
        player->SetSurface(nativeWindow);
        if (callback) {
            jclass clazz = env->GetObjectClass(obj);
            jmethodID javaCallback = env->GetMethodID(clazz, "onFrameCallBack", "([BII)V");
            if (javaCallback != NULL) {
                player->SetCallBack(javaCallback);
            } else {
                ALOGW("not found callback onFrameCallBack([BII)V");
            }
        }
    }
}

void jni_set_datasource(JNIEnv *env, jobject obj, jlong ptr, jstring path) {
    if (ptr != 0) {
        kk::VideoPlayer *player = (kk::VideoPlayer *) ptr;
        jboolean b = static_cast<jboolean>(true);
        const char *filename = env->GetStringUTFChars(path, &b);
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
    }
}

void jni_ffmpeg_init(JNIEnv *env, jclass) {
    av_register_all();
}


JavaVM *gVm;

JNIEXPORT jint JNI_OnLoad(JavaVM *vm, void *) {
    gVm = vm;
    JNIEnv *env;
    vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
    jclass nativeEngineClass = (jclass) env->NewGlobalRef(env->FindClass(JNI_CLASS_NAME));
    static JNINativeMethod methods[] = {
            {"native_create",         "()J",                         (void *) jni_create_player},
            {"native_set_callback",   "(JLandroid/view/Surface;Z)V", (void *) jni_player_set_callback},
            {"native_set_datasource", "(JLjava/lang/String;)V",      (void *) jni_set_datasource},
            {"native_play",           "(J)I",                        (void *) jni_player_play},
            {"native_preload",        "(J)I",                        (void *) jni_player_preload},
            {"native_stop",           "(J)V",                        (void *) jni_player_stop},
            {"native_close",          "(J)V",                        (void *) jni_player_close},
            {"native_get_status",     "(J)I",                        (void *) jni_player_get_status},
            {"native_init_ffmpeg",    "()V",                         (void *) jni_ffmpeg_init}
    };
    if (env->RegisterNatives(nativeEngineClass, methods, 9) < 0) {
        return JNI_ERR;
    }
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNI_OnUnload(JavaVM *vm, void *reserved) {
    vm = NULL;
}

JNIEnv *getEnv() {
    JNIEnv *env;
    gVm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6);
    return env;
}

JNIEnv *ensureEnvCreated() {
    JNIEnv *env = getEnv();
    if (env == NULL) {
        gVm->AttachCurrentThread(&env, NULL);
    }
    return env;
}