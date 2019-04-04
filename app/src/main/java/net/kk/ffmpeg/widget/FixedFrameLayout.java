package net.kk.ffmpeg.widget;

import android.content.Context;
import android.content.res.TypedArray;
import android.util.AttributeSet;
import android.util.Log;
import android.widget.FrameLayout;

import com.max.ffmpegnativewindow.R;


public class FixedFrameLayout extends FrameLayout {
    private int mTargetWidth, mTargetHeight;
    private static final String TAG = "FixedLayout";

    public FixedFrameLayout(Context context) {
        this(context, null);
    }

    public FixedFrameLayout(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public FixedFrameLayout(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        if (attrs != null) {
            int[] arr = new int[]{R.styleable.FixedLayout_fixed_x, R.styleable.FixedLayout_fixed_y};
            TypedArray typedArray = context.obtainStyledAttributes(attrs, arr);
            if (typedArray != null) {
                mTargetWidth = typedArray.getInt(0, 9);
                mTargetHeight = typedArray.getInt(1, 16);
                typedArray.recycle();
            }
        }
    }

    public void setTargetSize(int targetWidth, int targetHeight) {
        if(targetWidth >0 && targetHeight > 0) {
            mTargetWidth = targetWidth;
            mTargetHeight = targetHeight;
            Log.d(TAG, "setTargetSize:" + targetHeight + "x" + targetHeight);
            int wsp = MeasureSpec.makeMeasureSpec(targetWidth, MeasureSpec.EXACTLY);
            int hsp = MeasureSpec.makeMeasureSpec(targetHeight, MeasureSpec.EXACTLY);
            measure(wsp, hsp);
        }
    }

    @Override
    protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
        int maxWidth = getDefaultSize(0, widthMeasureSpec);
        int maxHeight = getDefaultSize(0, heightMeasureSpec);
        int w1 = maxWidth / 10 * 10;
        int h1 = Math.round((float) w1 / (float) mTargetWidth * (float) mTargetHeight);
        int h2 = maxHeight / 10 * 10;
        int w2 = Math.round((float) h2 / (float) mTargetHeight * (float) mTargetWidth);
        Log.i(TAG, "X=" + mTargetWidth + ",Y=" + mTargetHeight + ", maxWidth=" + maxWidth + ",maxHeight=" + maxHeight + ", w1=" + w1 + ",h1=" + h1 + ",  w2=" + w2 + ",h2=" + h2);
        if (w2 <= maxWidth) {
            super.onMeasure(MeasureSpec.makeMeasureSpec(w2, MeasureSpec.EXACTLY), MeasureSpec.makeMeasureSpec(h2, MeasureSpec.EXACTLY));
        } else if (h1 <= maxHeight) {
            super.onMeasure(MeasureSpec.makeMeasureSpec(w1, MeasureSpec.EXACTLY), MeasureSpec.makeMeasureSpec(h1, MeasureSpec.EXACTLY));
        } else {
            super.onMeasure(widthMeasureSpec, heightMeasureSpec);
        }
    }
}
