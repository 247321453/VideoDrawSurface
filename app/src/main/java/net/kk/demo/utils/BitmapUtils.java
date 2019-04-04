package net.kk.demo.utils;

import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Matrix;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.view.View;
import android.widget.ImageView;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

@SuppressWarnings({"unused", "WeakerAccess"})
public class BitmapUtils {
    public static void dispose(View v) {
        if (v == null)
            return;
        if (v instanceof ImageView) {
            dispose(((ImageView) v).getDrawable());
        }
        dispose(v.getBackground());
    }

    public static void dispose(Bitmap bitmap) {
        if (bitmap == null || bitmap.isRecycled())
            return;
        bitmap.recycle();
    }

    public static void dispose(Drawable drawable) {
        if (drawable != null) {
            drawable.setCallback(null);
            if (drawable instanceof BitmapDrawable) {
                BitmapDrawable bitmapDrawable = (BitmapDrawable) drawable;
                dispose(bitmapDrawable.getBitmap());
            }
        }
    }

    public static Bitmap getBitmapByFile(String file) {
        InputStream input = null;
        Bitmap b = null;
        try {
            input = new FileInputStream(file);
            b = getBitmapByStream(input);
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                if (input != null)
                    input.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        return b;
    }

    public static Bitmap getBitmapByStream(InputStream drawinput) {
        BitmapFactory.Options moptions = new BitmapFactory.Options();
        moptions.inJustDecodeBounds = false;
        moptions.inPurgeable = true;
        moptions.inInputShareable = true;
        return BitmapFactory.decodeStream(drawinput, null, moptions);
    }

    @SuppressWarnings("ResultOfMethodCallIgnored")
    public static boolean saveJpeg(Bitmap bitmap, String file) {
        File img = new File(file);
        File tmp = new File(file + ".tmp");
        if (!img.exists()) {
            //img.delete();
            File dir = img.getParentFile();
            if (!dir.exists()) {
                dir.mkdirs();
            }
        }
        FileOutputStream outputStream = null;
        try {
            outputStream = new FileOutputStream(tmp);
            bitmap.compress(Bitmap.CompressFormat.JPEG, 100, outputStream);
            if (img.exists()) {
                img.delete();
            }
            return tmp.renameTo(img);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        } finally {
            if (outputStream != null) {
                try {
                    outputStream.close();
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
        }
        return false;
    }

    /***
     *
     * @param file 文件
     * @return 图片位图
     * @throws OutOfMemoryError 抛出的异常
     */
    public static Bitmap read(String file, boolean hasAlpha) throws OutOfMemoryError {
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inPreferredConfig = hasAlpha ? Bitmap.Config.ARGB_8888 : Bitmap.Config.RGB_565;
        options.inJustDecodeBounds = true;
        BitmapFactory.decodeFile(file, options);
        options.inJustDecodeBounds = false;
        int bmpW = options.outWidth;
        int bmpH = options.outHeight;

        int screenW = Resources.getSystem().getDisplayMetrics().widthPixels;
        int screenH = Resources.getSystem().getDisplayMetrics().heightPixels;

        if (!hasAlpha) {
            screenW = screenW * 2;
            screenH = screenH * 2;
        }
        //分别根据最大边，得到图片
        Bitmap bmp;
        if ((bmpW * bmpH) > (screenW * screenH)) {
            options.inSampleSize = getSampleSize(bmpW, bmpH, screenW, screenH);
        }
        bmp = BitmapFactory.decodeFile(file, options);
        return bmp;
    }


    /***
     *
     * @param file 文件
     * @return 图片位图
     * @throws OutOfMemoryError 抛出的异常
     */
    public static Bitmap readWithCat(String file, int cameraW, int cameraH) throws OutOfMemoryError {
        BitmapFactory.Options options = new BitmapFactory.Options();
        options.inPreferredConfig = Bitmap.Config.RGB_565;
        options.inJustDecodeBounds = true;
        BitmapFactory.decodeFile(file, options);
        options.inJustDecodeBounds = false;
        int bmpW = options.outWidth;
        int bmpH = options.outHeight;

        int screenW = Resources.getSystem().getDisplayMetrics().widthPixels;
        int screenH = Resources.getSystem().getDisplayMetrics().heightPixels;


        boolean screenV = screenW < screenH;
        //3,4
        boolean cameraV = cameraW < cameraH;

        int angle;

        int targetW, targetH;
        if (cameraV == screenV) {
            angle = 0;
            targetW = cameraW;
            targetH = cameraH;
        } else {
            angle = 90;
            targetW = cameraH;
            targetH = cameraW;
        }
        Bitmap target;
        //分别根据最大边，得到图片
        Bitmap bmp;
        if (bmpW == targetW && bmpH == targetH) {
            bmp = BitmapFactory.decodeFile(file, options);
        } else {
            if ((bmpW * bmpH) > (targetW * targetH)) {
                options.inSampleSize = getSampleSize(bmpW, bmpH, targetW, targetH);
            }
            bmp = BitmapFactory.decodeFile(file, options);
        }
        if (bmp == null) {
            return null;
        }
        //
        Bitmap catBmp;
        if (angle != 0) {
            //反向旋转，再裁剪
            Matrix matrix = new Matrix();
            matrix.reset();
            matrix.postRotate(-angle);
            Bitmap bMapRotate = Bitmap.createBitmap(bmp, 0, 0, bmp.getWidth(), bmp.getHeight(), matrix, true);
            if (bMapRotate != bmp) {
                dispose(bmp);
            }
            catBmp = catBitmap(bMapRotate, targetW, targetH);
            if (bMapRotate != catBmp) {
                dispose(bMapRotate);
            }
            matrix.reset();
            matrix.postRotate(angle);
            Bitmap rs = Bitmap.createBitmap(catBmp, 0, 0, catBmp.getWidth(), catBmp.getHeight(), matrix, true);
            if (rs != catBmp) {
                dispose(catBmp);
            }
            return rs;
        } else {
            //直接裁剪
            catBmp = catBitmap(bmp, targetW, targetH);
            if (catBmp != bmp) {
                dispose(bmp);
            }
            return catBmp;
        }
    }

    /**
     * @param width  目标宽
     * @param height 目标高
     */
    @SuppressWarnings("UnnecessaryLocalVariable")
    private static Bitmap catBitmap(Bitmap bitmap, int width, int height) {
        int w = bitmap.getWidth();
        int h = bitmap.getHeight();
        if (w == width && h == height) {
            return bitmap;
        }
        //h1 w1
        //h w    w1/w*h = h1
        int w1 = width;
        int h1 = Math.round((float) w1 / (float) w * (float) h);

        //w w2
        //h h2  (w/h)*h2 = w2
        int h2 = height;
        int w2 = Math.round((float) w / (float) h * (float) h2);

        if (h1 > height) {
            Bitmap createScaledBitmap = Bitmap.createScaledBitmap(bitmap, w1, h1, true);
            //裁剪
            if (createScaledBitmap != bitmap) {
                dispose(bitmap);
            }
            Bitmap resize = Bitmap.createBitmap(createScaledBitmap, 0, (h1 - height) / 2, w1, height);
            if (createScaledBitmap != resize) {
                dispose(createScaledBitmap);
            }
            return resize;
        } else if (w2 > width) {
            Bitmap createScaledBitmap = Bitmap.createScaledBitmap(bitmap, w2, h2, true);
            //裁剪
            if (createScaledBitmap != bitmap) {
                dispose(bitmap);
            }
            Bitmap resize = Bitmap.createBitmap(createScaledBitmap, (w2 - width) / 2, 0, width, h2);
            if (createScaledBitmap != resize) {
                dispose(createScaledBitmap);
            }
            return resize;
        } else {
            Bitmap createScaledBitmap = Bitmap.createScaledBitmap(bitmap, width, height, true);
            if (createScaledBitmap != bitmap) {
                dispose(bitmap);
            }
            return createScaledBitmap;
        }
    }

    private static int getSampleSize(int w, int h, int W, int H) {
        float f = 1.0f;
        while (((double) (f * 2.0f)) <= Math.min(((double) w) / ((double) W), ((double) h) / ((double) H))) {
            f *= 2.0f;
        }
        return (int) f;
    }
}
