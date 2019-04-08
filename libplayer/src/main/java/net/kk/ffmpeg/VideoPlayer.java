package net.kk.ffmpeg;

import android.os.Handler;
import android.os.HandlerThread;
import android.support.annotation.Keep;
import android.support.annotation.WorkerThread;
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
        void onVideoProgress(double cur, double all);

        void onPlayStart();

        void onPlayEnd(int error);

        void onVideoPreLoad(int error, int width, int heigh, int rotate, double allTime);

        void onFrameCallBack(byte[] nv21Data, int width, int height);
    }

    static {
        System.loadLibrary("kkplayer");
    }

    private HandlerThread mWorkThread;
    private Handler mHandler;
    private long nativePtr;
    private CallBack mCallBack;
    private double mCurTime;
    private double mAllTime;
    private String source;

    public VideoPlayer() {
        nativePtr = native_create();
    }

    /**
     * @param width
     * @param height
     * @param stretch 是否拉伸
     * @param rotate  0-3
     * @see Surface#ROTATION_0,Surface#ROTATION_90,Surface#ROTATION_180,Surface#ROTATION_270
     */
    public void setSize(int width, int height, boolean stretch, int rotate) {
        native_set_size(nativePtr, width, height, stretch, rotate);
    }

    public void setCallback(CallBack dataCallBack, boolean needNv21Data) {
        mCallBack = dataCallBack;
        native_set_callback(nativePtr, needNv21Data);
    }

    public void setSurface(Surface surface) {
        native_set_surface(nativePtr, surface);
    }

    public String getSource() {
        return source;
    }

    public void setDataSource(String path) {
        source = path;
        native_set_data_source(nativePtr, path);
    }

    public void play() {
        init();
        post(new Runnable() {
            @Override
            public void run() {
                if (mCallBack != null) {
                    mCallBack.onPlayStart();
                }
                int ret = -20;
                try {
                    ret = native_play(nativePtr);
                }catch (Throwable e){
                    e.printStackTrace();
                }
                if (mCallBack != null) {
                    mCallBack.onPlayEnd(ret);
                }
            }
        });
    }

    public void preload(final boolean autoPlay) {
        init();
        post(new Runnable() {
            @Override
            public void run() {
                int ret = -20;
                try {
                    ret = native_preload(nativePtr);
                    mAllTime = getPlayTime();
                }catch (Throwable e){
                    e.printStackTrace();
                }
                if (mCallBack != null) {
                    mCallBack.onVideoPreLoad(ret, getVideoWidth(), getVideoHeight(), getVideoRotate(), getVideoTime());
                }
                if(ret == 0 && autoPlay){
                    play();
                }
            }
        });
    }

    public int getVideoWidth() {
        return native_get_width(nativePtr);
    }

    public int getVideoHeight() {
        return native_get_height(nativePtr);
    }

    public int getVideoRotate() {
        return native_get_rotate(nativePtr);
    }

    public void release() {
        native_release(nativePtr);
    }

    private void closeThread() {
        if (mWorkThread != null && !mWorkThread.isInterrupted()) {
            mWorkThread.interrupt();
        }
        mWorkThread = null;
        mHandler = null;
    }

    private void checkThread() {
        if (mWorkThread == null || mWorkThread.isInterrupted()) {
            mWorkThread = new HandlerThread("kk_player_work");
            mWorkThread.start();
            mHandler = new Handler(mWorkThread.getLooper());
        }
    }
    private void post(Runnable runnable){
        checkThread();
        mHandler.post(runnable);
    }

    public void stop() {
        native_stop(nativePtr);
    }

    @Override
    public void close() {
        native_close(nativePtr);
    }

    public int seek(double ms) {
        return native_seek(nativePtr, ms);
    }

    public double getPlayTime() {
        return native_get_cur_time(nativePtr);
    }

    public double getVideoTime() {
        return native_get_all_time(nativePtr);
    }

    @Keep
    public void onFrameCallBack(byte[] nv21Data, int width, int height, double progress, double alltime) {
        if (mCallBack != null) {
            mCallBack.onVideoProgress(progress, alltime);
            if (nv21Data != null) {
                mCallBack.onFrameCallBack(nv21Data, width, height);
            }
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

//    public static int testPlay(Surface surface, String file){
//        return native_test_play(surface, file);
//    }

    private static boolean sInit;

    private static native void native_init_ffmpeg();

    private static native long native_create();

    private native void native_set_surface(long ptr, Surface surface);

    private native void native_set_callback(long ptr, boolean callback);

    private native void native_set_size(long ptr, int width, int height, boolean stretch, int rotation);

    private native void native_set_data_source(long ptr, String path);

    private native int native_play(long ptr);

    private native int native_preload(long ptr);

    private native int native_get_status(long ptr);

    private native void native_stop(long ptr);

    private native void native_close(long ptr);

    private native void native_release(long ptr);

    private native double native_get_cur_time(long ptr);

    private native double native_get_all_time(long ptr);

    private native int native_seek(long ptr, double ms);

    private native int native_get_width(long ptr);

    private native int native_get_height(long ptr);

    private native int native_get_rotate(long ptr);

    /***
     * 原始 i420
     */
    private native int native_get_last_frame(long ptr, byte[] yuvData);

    private native int native_get_last_argb_image(long ptr, byte[] argb, int width, int height,int rotation);
//    private static native int native_test_play(Surface surface, String path);
}
