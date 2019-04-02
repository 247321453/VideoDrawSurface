package com.max.ffmpegnativewindow;

import android.Manifest;
import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.graphics.YuvImage;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
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
        player.setCallBack(surfaceHolder.getSurface(), this);
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    public void onClicked(View view) {
        if (!created) {
            Log.w(TAG, "surface not created, now return. ");
            return;
        }
        final String path = editText.getText().toString();
        view.setEnabled(false);
        editText.setEnabled(false);
        new Thread(new Runnable() {
            @Override
            public void run() {
                VideoPlayer.testPlay(surfaceHolder.getSurface(), path);
            }
        }).start();
        /*
        player.setDataSource(path);
        if (player.isPlaying()) {
            ((TextView) view).setText("play");
            if (player.isPlaying()) {
                player.stop();
            }
        } else {
            ((TextView) view).setText("stop");
            new Thread(new Runnable() {
                @Override
                public void run() {
                    int err = player.preload();
                    if (err != 0) {
                        Log.e(TAG, "preload:" + err);
                        return;
                    }
                    err = player.play();
                    Log.e(TAG, "play:" + err);
                }
            }).start();
        }
        view.setEnabled(true);
        */
    }

    private boolean mShowed;

    @Override
    public void onFrameCallBack(byte[] nv21Data, int width, int height) {
        if (mShowed) {
            return;
        }
        mShowed = true;
        YuvImage yuvImage = new YuvImage(nv21Data, ImageFormat.NV21, width, height, null);
        ByteArrayOutputStream fOut = new ByteArrayOutputStream();
        yuvImage.compressToJpeg(new Rect(0, 0, width, height), 100, fOut);
        byte[] bitData = fOut.toByteArray();
        final Bitmap bitmap = BitmapFactory.decodeByteArray(bitData, 0, bitData.length);
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                img.setImageBitmap(bitmap);
            }
        });
    }
}
