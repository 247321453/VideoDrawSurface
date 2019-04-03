package com.max.ffmpegnativewindow;

import android.Manifest;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.graphics.YuvImage;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;

import net.kk.ffmpeg.VideoPlayer;

import java.io.ByteArrayOutputStream;

public class MainActivity extends BaseActivity implements SurfaceHolder.Callback, VideoPlayer.CallBack {

    private SurfaceView surfaceView;
    private SurfaceHolder surfaceHolder;
    private EditText editText;
    private boolean created = false;
    private static final String TAG = "kkplayer";

    private ImageView img;
    private VideoPlayer player;

    @Override
    protected String[] getRequestPermissions() {
        return new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE};
    }

    @Override
    protected void doOnCreate(Bundle savedInstanceState) {
        super.doOnCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        editText = findViewById(R.id.file_path);
        img = findViewById(R.id.img);
        surfaceView = (SurfaceView) findViewById(R.id.videoSurface);
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);
        player = new VideoPlayer();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        created = true;
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    @Override
    protected void onDestroy() {
        player.close();
        super.onDestroy();
    }

    public void onClicked(final View view) {
        if (!created) {
            Log.w(TAG, "surface not created, now return. ");
            return;
        }
        if(!player.isInit()) {
            //        player.init(surfaceHolder.getSurface(), 0, 0, this, true);
            player.init(surfaceHolder.getSurface(), 0, 0, false, this, true);
        }
        final String path = editText.getText().toString();
        view.setEnabled(false);
        editText.setEnabled(false);
//        new Thread(new Runnable() {
//            @Override
//            public void run() {
//                VideoPlayer.testPlay(surfaceHolder.getSurface(), path);
//            }
//        }).start();
        if (!TextUtils.equals(player.getSource(), path)) {
            player.setDataSource(path);
        }
        if (player.isPlaying()) {
            ((TextView) view).setText("play");
            if (player.isPlaying()) {
                player.stop();
            }
            editText.setEnabled(true);
        } else {
            ((TextView) view).setText("stop");
            new Thread(new Runnable() {
                @Override
                public void run() {
                    try {
                        Thread.sleep(100);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                    }
                    int err = player.preload();
                    if (err < 0) {
                        Log.e(TAG, "preload:" + err);
                        return;
                    }
                    Log.i(TAG, "video time:" + player.getVideoTime());
                    player.seek(0);
                    err = player.play();
                    if (err != 0) {
                        Log.e(TAG, "play:" + err);
                    }
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            ((TextView) view).setText("play");
                        }
                    });
                }
            }).start();
        }
        view.setEnabled(true);
    }

    private long time;

    @Override
    public void onVideoProgress(double cur, double all) {
        //时间
    }

    @Override
    public void onFrameCallBack(byte[] nv21Data, final int width, final int height) {
        if (System.currentTimeMillis() - time > 500) {
            time = System.currentTimeMillis();
            YuvImage yuvImage = new YuvImage(nv21Data, ImageFormat.NV21, width, height, null);
            ByteArrayOutputStream fOut = new ByteArrayOutputStream();
            yuvImage.compressToJpeg(new Rect(0, 0, width, height), 100, fOut);
            byte[] bitData = fOut.toByteArray();
            final Bitmap bitmap = BitmapFactory.decodeByteArray(bitData, 0, bitData.length);
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    Log.d(TAG, "update image " + width + "x" + height);
                    BitmapUtils.dispose(img);
                    img.setImageBitmap(bitmap);
                }
            });
        }
    }
}
