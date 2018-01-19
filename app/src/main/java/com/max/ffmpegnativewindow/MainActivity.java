package com.max.ffmpegnativewindow;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.EditText;

public class MainActivity extends Activity implements SurfaceHolder.Callback{

    private SurfaceView surfaceView;
    private SurfaceHolder surfaceHolder;
    private EditText editText;
    private boolean created = false;
    private static final String TAG = "maxD";

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        editText = findViewById(R.id.file_path);

        surfaceView = (SurfaceView) findViewById(R.id.videoSurface);
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);
    }

    private native int native_play(Surface surface);
    private native void native_setFile(String filepath);

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

    public void onClicked(View view){

        if (!created){
            Log.w(TAG, "surface not created, now return. ");
            return;
        }
        String path = editText.getText().toString();
        native_setFile(path);

        new Thread(new Runnable() {
            @Override
            public void run() {
                native_play(surfaceHolder.getSurface());
            }
        }).start();
    }
}
