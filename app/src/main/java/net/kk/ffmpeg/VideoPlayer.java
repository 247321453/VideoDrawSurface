package net.kk.ffmpeg;

import android.view.Surface;

import java.io.Closeable;

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
public class VideoPlayer implements Closeable {

    public interface CallBack {
        void onFrameCallBack(byte[] nv21Data, int width, int height);
    }

    static {
        System.loadLibrary("kkplayer");
    }

    private long nativePtr;
    private CallBack mCallBack;

    public VideoPlayer() {
        nativePtr = native_create();
    }

    public void setCallBack(Surface surface, CallBack dataCallBack) {
        mCallBack = dataCallBack;
        native_set_callback(nativePtr, surface, dataCallBack != null);
    }

    public void setDataSource(String path) {
        native_set_datasource(nativePtr, path);
    }

    public int play() {
        init();
        return native_play(nativePtr);
    }

    public int preload() {
        init();
        return native_preload(nativePtr);
    }

    public void stop() {
        native_stop(nativePtr);
    }

    @Override
    public void close() {
        native_close(nativePtr);
    }

    public void onFrameCallBack(byte[] nv21Data, int width, int height) {
        if (mCallBack != null) {
            mCallBack.onFrameCallBack(nv21Data, width, height);
        }
    }

    public boolean isPlaying() {
        return native_get_status(nativePtr) == 1;
    }

    private static void init() {
        if (!sInit) {
            sInit = true;
            native_init_ffmpeg();
        }
    }

    public static int testPlay(Surface surface, String file){
        return native_test_play(surface, file);
    }

    private static boolean sInit;

    private static native void native_init_ffmpeg();

    private static native long native_create();

    private native void native_set_callback(long ptr, Surface surface, boolean callback);

    private native void native_set_datasource(long ptr, String path);

    private native int native_play(long ptr);

    private native int native_preload(long ptr);

    private native int native_get_status(long ptr);

    private native void native_stop(long ptr);

    private native void native_close(long ptr);

    private static native int native_test_play(Surface surface, String path);
}
