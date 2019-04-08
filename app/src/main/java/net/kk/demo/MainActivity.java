package net.kk.demo;

import android.Manifest;
import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.ImageFormat;
import android.graphics.Rect;
import android.graphics.YuvImage;
import android.os.Bundle;
import android.os.Looper;
import android.support.v7.widget.AppCompatSpinner;
import android.text.TextUtils;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;

import com.max.ffmpegnativewindow.R;

import net.kk.demo.ui.BaseAdapterPlus;
import net.kk.demo.ui.widget.FixedFrameLayout;
import net.kk.demo.utils.BitmapUtils;
import net.kk.ffmpeg.VideoPlayer;

import java.io.ByteArrayOutputStream;
import java.util.ArrayList;
import java.util.List;

public class MainActivity extends BaseActivity implements SurfaceHolder.Callback, VideoPlayer.CallBack {

    private SurfaceView surfaceView;
    private SurfaceHolder surfaceHolder;
    private EditText editText;
    private boolean created = false;
    private static final String TAG = "kkplayer";

    private ImageView img;
    private VideoPlayer player;
    private FixedFrameLayout mFixedFrameLayout;
    private MyAdapter mMyAdapter;
    private AppCompatSpinner testSp;
    private Button btnPlay;

    @Override
    protected String[] getRequestPermissions() {
        return new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE};
    }

    private TestInfo mTestInfo, mLastInfo;

    @Override
    protected void doOnCreate(Bundle savedInstanceState) {
        super.doOnCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mFixedFrameLayout = findViewById(R.id.ly_fixed);
        editText = findViewById(R.id.file_path);
        testSp = findViewById(R.id.sp_test_list);
        btnPlay = findViewById(R.id.btn_play);
        mMyAdapter = new MyAdapter(this);
        mMyAdapter.set(initItems());
        testSp.setAdapter(mMyAdapter);
        testSp.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
            @Override
            public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                mLastInfo = mTestInfo;
                mTestInfo = mMyAdapter.getDataItem(position);
                player.seek(0);
            }

            @Override
            public void onNothingSelected(AdapterView<?> parent) {

            }
        });
        mTestInfo = mMyAdapter.getDataItem(0);
        img = findViewById(R.id.img);
        surfaceView = (SurfaceView) findViewById(R.id.videoSurface);
        surfaceHolder = surfaceView.getHolder();
        surfaceHolder.addCallback(this);
        player = new VideoPlayer();
        player.setCallback(this, true);
    }

    private List<TestInfo> initItems() {
        List<TestInfo> items = new ArrayList<>();
        items.add(new TestInfo("正常", 0, 0, false, Surface.ROTATION_0));
        items.add(new TestInfo("顺时针旋转90度", 0, 0, false, Surface.ROTATION_90));
        items.add(new TestInfo("逆时针旋转90度", 0, 0, false, Surface.ROTATION_270));
        items.add(new TestInfo("4：3画面拉伸", 900, 1200, true, Surface.ROTATION_0));
        items.add(new TestInfo("按4：3比例缩放", 900, 1200, false, Surface.ROTATION_0));
        items.add(new TestInfo("宽高对换测试1", 1280, 720, false, Surface.ROTATION_0));
        items.add(new TestInfo("宽高对换测试2", 1280, 720, false, Surface.ROTATION_90));
        items.add(new TestInfo("宽高对换测试3", 1280, 720, false, Surface.ROTATION_270));
        return items;
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        created = true;
        player.setSurface(surfaceHolder.getSurface());
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }

    @Override
    protected void onDestroy() {
        player.close();
        super.onDestroy();
    }

    private void updatePlay(final boolean play) {
        if (Looper.getMainLooper() != Looper.myLooper()) {
            runOnUiThread(() -> updatePlay(play));
            return;
        }
        testSp.setEnabled(!play);
        editText.setEnabled(!play);
        if (play) {
            btnPlay.setText("stop");
        } else {
            btnPlay.setText("play");
        }
    }

    public void onClicked(final View view) {
        if (!created) {
            Log.w(TAG, "surface not created, now return. ");
            return;
        }

        final String path = editText.getText().toString();
        view.setEnabled(false);
        if (player.isPlaying()) {
            if (player.isPlaying()) {
                player.stop();
                //测试截图
                BitmapUtils.dispose(img);
                img.setImageBitmap(null);
                img.setImageResource(android.R.drawable.sym_def_app_icon);
                player.takeImage();
            }
            updatePlay(false);
        } else {
            if (!TextUtils.equals(player.getSource(), path)) {
                player.setDataSource(path);
                player.preload(false);
            } else {
                startPlay(player.getVideoWidth(), player.getVideoHeight(), player.getVideoRotate());
            }
        }
        view.setEnabled(true);
    }

    private long time;

    @Override
    public void onPlayStart() {

    }

    @Override
    public void onPlayEnd(int error) {
        updatePlay(false);
        if (error != 0) {
            Log.e(TAG, "play:" + error);
        }
    }

    @Override
    public void onVideoPreLoad(int error, int width, int height, int rotate, double allTime) {
        if (error != 0) {
            updatePlay(false);
            Log.e(TAG, "preload:" + error);
        } else {
            startPlay(width, height, rotate);
        }
    }

    private void startPlay(int width, int height, int rotate) {
        if (mTestInfo != null) {
            //跳转播放时间
            player.setSize(mTestInfo.width, mTestInfo.height, mTestInfo.stretch, mTestInfo.rotation);
            int w, h;
            int r = (mTestInfo.rotation + rotate) % 4;
            if (mTestInfo.width == 0) {
                w = r % 2 == 0 ? width : height;
            } else {
                w = mTestInfo.width;
            }
            if (mTestInfo.height == 0) {
                h = r % 2 == 0 ? height : width;
            } else {
                h = mTestInfo.height;
            }
            Log.i("FixedLayout", "video size=" + width + "x" + height+", target="+w+"x"+h);
            runOnUiThread(() -> {
                updatePlay(true);
                mFixedFrameLayout.setTargetSize(w, h);
                player.play();
            });
        }
    }

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
            runOnUiThread(() -> {
                Log.d(TAG, "onFrameCallBack update image " + width + "x" + height);
                BitmapUtils.dispose(img);
                img.setImageBitmap(bitmap);
            });
        }
    }

    @Override
    public void onTakeImageCallBack(byte[] nv21Data, int width, int height) {
        YuvImage yuvImage = new YuvImage(nv21Data, ImageFormat.NV21, width, height, null);
        ByteArrayOutputStream fOut = new ByteArrayOutputStream();
        yuvImage.compressToJpeg(new Rect(0, 0, width, height), 100, fOut);
        byte[] bitData = fOut.toByteArray();
        final Bitmap bitmap = BitmapFactory.decodeByteArray(bitData, 0, bitData.length);
        runOnUiThread(() -> {
            Log.d(TAG, "onTakeImageCallBack update image " + width + "x" + height);
            BitmapUtils.dispose(img);
            img.setImageBitmap(bitmap);
        });
    }

    private class TestInfo {
        String name;
        int width;
        int height;
        boolean stretch;
        int rotation;

        public TestInfo(String name, int width, int height, boolean stretch, int rotaion) {
            this.name = name;
            this.width = width;
            this.height = height;
            this.stretch = stretch;
            this.rotation = rotaion;
        }

        @Override
        public String toString() {
            return name + " " + ((width > 0 && height > 0) ? (width + "x" + height) + "/" : "")
                    + (stretch ? "stretch" : "fixed")
                    + " " + (rotation * 90) + "°";
        }
    }

    private class MyHolder {
        final TextView text;

        MyHolder(View view) {
            text = view.findViewById(R.id.text1);
        }
    }

    private class MyAdapter extends BaseAdapterPlus<TestInfo> {
        public MyAdapter(Context context) {
            super(context);
        }

        @Override
        protected View createView(int position, ViewGroup parent) {
            View view = inflate(R.layout.item_spinner, parent, false);
            MyHolder myHolder = new MyHolder(view);
            view.setTag(myHolder);
            return view;
        }

        @Override
        protected void attach(View view, TestInfo item, int position) {
            MyHolder myHolder = (MyHolder) view.getTag();
            myHolder.text.setText(item.toString());
        }
    }
}
