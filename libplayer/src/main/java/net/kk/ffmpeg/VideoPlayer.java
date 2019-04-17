package net.kk.ffmpeg;

import android.graphics.SurfaceTexture;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.support.annotation.Keep;
import android.text.TextUtils;
import android.util.Log;
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

    private static final boolean DEBUG = true;

    public interface CallBack {
        void onVideoProgress(double cur, double all);

        void onPlayStart();

        void onPlayEnd(int error);

        void onVideoPreLoad(int error, int width, int heigh, int rotate, double allTime);

        void onTakeImageCallBack(byte[] nv21Data, int width, int height);

        void onFrameCallBack(byte[] nv21Data, int width, int height);
    }

    static {
        System.loadLibrary("kkplayer");
    }

    private static final String TAG = "kkplayer";
    private HandlerThread mPlayThread;
    private MyHandler mPlayHandler;
    private HandlerThread mCallBackThread;
    private Handler mCallBackHandler;
    private long nativePtr;
    private CallBack mCallBack;
    private double mCurTime;
    private double mAllTime;
    private String source;
    private boolean mClose;
    private Surface mTexSurface;
    private Surface mSurface;
    private SurfaceTexture mTexture;

    public VideoPlayer() {
        this(false);
    }

    /**
     * @param rgb565 true则用rgb565，false则用rgba
     */
    public VideoPlayer(boolean rgb565) {
        nativePtr = native_create(rgb565);
    }

    /**
     * 要停止播放
     * @param width
     * @param height
     * @param stretch  是否拉伸
     * @param rotation 0-3
     * @see Surface#ROTATION_0,Surface#ROTATION_90,Surface#ROTATION_180,Surface#ROTATION_270
     */
    public void setSize(final int width, final int height, final boolean stretch, final int rotation, final int preview_rotation) {
        if (isPlaying()) {
            stop();
        }
        postPlayer(new Runnable() {
            @Override
            public void run() {
                native_set_size(nativePtr, width, height, stretch, rotation, preview_rotation);
            }
        }, "setSize");
    }

    public void setCallback(CallBack dataCallBack, boolean needNv21Data) {
        mCallBack = dataCallBack;
        native_set_callback(nativePtr, needNv21Data);
    }

    /**
     * 要停止播放
     * @param surface
     * @param texture
     */
    public void setSurface(Surface surface, SurfaceTexture texture) {
        if (surface == null && texture != null) {
            if (mTexture != texture) {
                releaseTexture();
                mTexSurface = new Surface(texture);
                mTexture = texture;
            }
            surface = mTexSurface;
        }
        if (surface != mSurface) {
            mSurface = surface;
            final Surface _surface = surface;
            postPlayer(new Runnable() {
                @Override
                public void run() {
                    native_set_surface(nativePtr, _surface);
                }
            }, "setSurface");
        }
    }

    public String getSource() {
        return source;
    }

    /**
     * 要停止播放
     * @param path
     */
    public void setDataSource(final String path) {
        if (!TextUtils.equals(source, path)) {
            mAllTime = 0;
            source = path;
            if (isPlaying()) {
                stop();
            }
            postPlayer(new Runnable() {
                @Override
                public void run() {
                    native_set_data_source(nativePtr, path);
                }
            }, "setDataSource");
        }
    }

    public void play() {
        if (mClose) {
            return;
        }
        init();
        checkThread();
        if (DEBUG) {
            Log.d(TAG, "post play");
        }
        mPlayHandler.sendEmptyMessage(msg_play);
    }

    private void playInner() {
        if (DEBUG) {
            Log.d(TAG, "playInner");
        }
        if (isPlaying()) {
            return;
        }
        int ret = -30;
        if (mCallBack != null) {
            mCallBack.onPlayStart();
        }
        try {
            ret = native_play(nativePtr);
        } catch (Throwable e) {
            e.printStackTrace();
        }
        if (mCallBack != null) {
            mCallBack.onPlayEnd(ret);
        }
    }

    private int preLoadInner() {
        if (DEBUG) {
            Log.d(TAG, "preLoadInner");
        }
        int ret = -20;
        try {
            ret = native_preload(nativePtr);
            mAllTime = getPlayTime();
        } catch (Throwable e) {
            e.printStackTrace();
        }
        if (mCallBack != null) {
            mCallBack.onVideoPreLoad(ret, getVideoWidth(), getVideoHeight(), getVideoRotate(), getVideoTime());
        }
        return ret;
    }

    public void preload(final boolean autoPlay) {
        if (mClose) {
            return;
        }
        init();
        if (isPlaying()) {
            stop();
        }
        postPlayer(new Runnable() {
            @Override
            public void run() {
                int ret = preLoadInner();
                if (ret == 0 && autoPlay) {
                    playInner();
                }
            }
        }, "preload");
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

    private static final int msg_play = 1;
    private static final int msg_close = 2;

    private class MyHandler extends Handler {
        public MyHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case msg_play:
                    if (!isPlaying()) {
                        int ret;
                        if (mAllTime <= 0) {
                            if (DEBUG) {
                                Log.d(TAG, "postPlayer3 task start:preload");
                            }
                            ret = preLoadInner();
                            if (DEBUG) {
                                Log.d(TAG, "postPlayer3 task end:preload");
                            }
                        } else {
                            ret = 0;
                        }
                        if (DEBUG) {
                            Log.d(TAG, "postPlayer3 task start:play");
                        }
                        if (ret == 0) {
                            playInner();
                        }
                        if (DEBUG) {
                            Log.d(TAG, "postPlayer3 task end:play");
                        }
                    } else {
                        if (DEBUG) {
                            Log.w(TAG, "is playing");
                        }
                    }
                    break;
                case msg_close:
                    if (DEBUG) {
                        Log.d(TAG, "postPlayer3 task start:close");
                    }
                    closeInner();
                    if (DEBUG) {
                        Log.d(TAG, "postPlayer3 task end:close");
                    }
                    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN_MR2) {
                        mPlayThread.quit();
                    }
                    break;
            }
            super.handleMessage(msg);
        }
    }

    private void checkThread() {
        if (mPlayThread == null) {
            String name = "kk_player_work_" + (nativePtr * 3 / 2);
            mPlayThread = new HandlerThread(name);
            mPlayThread.start();
            mPlayHandler = new MyHandler(mPlayThread.getLooper());
        }
        if (mCallBackThread == null) {
            String name = "kk_player_callback_" + (nativePtr * 3 / 2);
            mCallBackThread = new HandlerThread(name);
            mCallBackThread.start();
            mCallBackHandler = new Handler(mCallBackThread.getLooper());
        }
    }

    public void postPlayer(final Runnable runnable, final String name) {
        checkThread();
        if (mPlayHandler != null) {
            if (Looper.myLooper() == mPlayHandler.getLooper()) {
                if (DEBUG) {
                    Log.d(TAG, "postPlayer1 task start:" + name);
                }
                runnable.run();
                if (DEBUG) {
                    Log.d(TAG, "postPlayer1 task end:" + name);
                }
            } else {
                mPlayHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        if (DEBUG) {
                            Log.d(TAG, "postPlayer2 task start:" + name);
                        }
                        runnable.run();
                        if (DEBUG) {
                            Log.d(TAG, "postPlayer2 task end:" + name);
                        }
                    }
                });
            }
        }
    }

    public void postCallBack(Runnable runnable, String name) {
        if (mClose) {
            return;
        }
        checkThread();
        if (DEBUG) {
            Log.v(TAG, "postCallBack task:" + name);
        }
        if (mCallBackHandler != null) {
            if (Looper.myLooper() == mCallBackHandler.getLooper()) {
                runnable.run();
            } else {
                mCallBackHandler.post(runnable);
            }
        }
    }

    public void stop() {
        native_stop(nativePtr);
    }

    private void releaseTexture() {
        if (mTexSurface != null) {
            try {
                mTexSurface.release();
            } catch (Throwable e) {
                e.printStackTrace();
            }
            mTexSurface = null;
        }
    }

    @Override
    public void close() {
        mClose = true;
        stop();
        releaseTexture();
        mPlayHandler.sendEmptyMessage(msg_close);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
            mPlayThread.quitSafely();
            mCallBackThread.quitSafely();
        } else {
            //see msg_close
//            mPlayThread.quit();
            mCallBackThread.quit();
        }
    }

    private void closeInner() {
        if (nativePtr != 0) {
            native_close(nativePtr);
            nativePtr = 0;
        }
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

    /**
     * 截图
     *
     * @param data
     * @param width
     * @param height
     */
    @Keep
    public void onImageCallBack(final byte[] data, final int width, final int height) {
        postCallBack(new Runnable() {
            @Override
            public void run() {
                if (mCallBack != null) {
                    mCallBack.onTakeImageCallBack(data, width, height);
                }
            }
        }, "onImageCallBack");
    }

    @Keep
    public void onFrameCallBack(final byte[] nv21Data, final int width, final int height,
                                final double progress, final double alltime) {
        postCallBack(new Runnable() {
            @Override
            public void run() {
                if (mCallBack != null) {
                    mCallBack.onVideoProgress(progress, alltime);
                    if (nv21Data != null) {
                        mCallBack.onFrameCallBack(nv21Data, width, height);
                    }
                }
            }
        }, "onFrameCallBack");
    }

    public boolean takeImage() {
        return takeImage(0, 0, 0, false);
    }

    /**
     * @param width
     * @param height
     * @param rotation
     * @param mirror   270度才建议使用
     * @return
     */
    public boolean takeImage(int width, int height, int rotation, boolean mirror) {
        stop();
        return native_take_image(nativePtr, width, height, rotation, mirror) == 0;
    }

    public void postTakeImage(final int width, final int height, final int rotation, final boolean mirror) {
        stop();
        postPlayer(new Runnable() {
            @Override
            public void run() {
                native_take_image(nativePtr, width, height, rotation, mirror);
            }
        }, "postTakeImage");
    }

    public boolean isPlaying() {
        return native_get_status(nativePtr) == 1;
    }

    public boolean isPause() {
        return false;
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

    private static native long native_create(boolean rgb565);

    private native void native_set_surface(long ptr, Surface surface);

    private native void native_set_callback(long ptr, boolean callback);

    private native void native_set_size(long ptr, int width, int height, boolean stretch, int rotation, int preview_rotation);

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

    private native int native_take_image(long ptr, int width, int height, int rotation, boolean mirror);

    /***
     * 原始 i420
     */
    private native int native_get_last_frame(long ptr, byte[] yuvData);

//    private static native int native_test_play(Surface surface, String path);
}
