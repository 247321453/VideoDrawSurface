package andhook.lib;

import android.os.Build;
import android.util.Log;

import java.io.File;
import java.lang.reflect.Member;
import java.lang.reflect.Modifier;

/**
 * @author Rprop
 * @version 3.6.2
 */
@SuppressWarnings({"unused", "WeakerAccess", "JniMissingFunction"})
public final class AndHook {
    private final static String LIB_NAME = "AK";
    public final static String VERSION = "3.6.2";
    public final static String LOG_TAG = "AndHook";

    @SuppressWarnings("all")
    public static void ensureNativeLibraryLoaded(final File lib_dir) {
        try {
            getVersionInfo();
            return;
        } catch (final UnsatisfiedLinkError ignored) {
        }

        // Check if there is a usable directory for temporary files
        final File tmpdir = new File(System.getProperty("java.io.tmpdir",
                "/data/local/tmp/"));
        if (!tmpdir.canWrite() || !tmpdir.canExecute())
            Log.w(LOG_TAG, "Missing cache directory " + tmpdir);

        try {
            if (lib_dir == null) {
                System.loadLibrary(LIB_NAME);
            } else {
                System.load(new File(lib_dir, "lib" + LIB_NAME + ".so")
                        .getAbsolutePath());
            }
        } catch (final UnsatisfiedLinkError e) {
            try {
                // compatible with libhoudini
                if (lib_dir == null) {
                    System.loadLibrary(LIB_NAME + "Compat");
                } else {
                    System.load(new File(lib_dir, "lib" + LIB_NAME + "Compat"
                            + ".so").getAbsolutePath());
                }
            } catch (final UnsatisfiedLinkError ignored) {
                throw new RuntimeException("Incompatible platform "
                        + android.os.Build.VERSION.SDK_INT, e);
            }
        }
    }

    public static native String getVersionInfo();

    public static native int backup(final Member origin);

    public static native int backup(final Class<?> clazz, final String name,
                                    final String signature);

    public static native boolean hook(final Member origin, final Object extra,
                                      int shared_backup);

    public static native boolean hook(final Class<?> clazz, final String name,
                                      final String signature, final Object extra, int shared_backup);

    public static int hook(final Member origin, final Object extra) {
        int slot = backup(origin);
        if (slot != -1) {
            if (!hook(origin, extra, slot))
                slot = -1;
        }
        return slot;
    }

    public static int hook(final Class<?> clazz, final String name,
                           final String signature, final Object extra) {
        int slot = backup(clazz, name, signature);
        if (slot != -1) {
            if (!hook(clazz, name, signature, extra, slot))
                slot = -1;
        }
        return slot;
    }

    public static void hookNoBackup(final Member origin, final Object extra) {
        hook(origin, extra, -1);
    }

    public static void hookNoBackup(final Class<?> clazz, final String name,
                                    final String signature, final Object extra) {
        hook(clazz, name, signature, extra, -1);
    }

    public static native boolean restore(final int slot, final Member origin);

    public static native boolean suspendAll();

    public static native void resumeAll();

    public static native void startDaemons();

    public static native void stopDaemons();

    public static native void disableLogging(final boolean state);

    private static native boolean initializeClass(final Class<?> clazz);

    @SuppressWarnings("all")
    public static boolean ensureClassInitialized(final Class<?> clazz) {
        if (clazz.isInterface() || Modifier.isAbstract(clazz.getModifiers())) {
            Log.w(LOG_TAG, "interface or abstract class `" + clazz.getName()
                    + "` cannot be initialized!");
            return false;
        }
        return initializeClass(clazz);
    }

    public static native void enableFastDexLoad(final boolean enable);

    public static native void optimizeMethod(final Member target);

    public static native void jitCompile(final Member target);

    public static native void deoptimizeMethod(final Member target);

    private static native void dumpClassMethods(final Class<?> clazz,
                                                final String clsname);

    public static void dumpClassMethods(final Class<?> clazz) {
        dumpClassMethods(clazz, null);
    }

    public static void dumpClassMethods(final String clsname) {
        dumpClassMethods(null, clsname);
    }

    public static native Object invoke(final int slot, final Object receiver,
                                       final Object... params);

    public static void invokeVoidMethod(final int slot, final Object receiver,
                                        final Object... params) {
        if (android.os.Build.VERSION.SDK_INT <= Build.VERSION_CODES.KITKAT_WATCH) {
            Dalvik.invokeVoidMethod(slot, receiver, params);
        } else {
            invoke(slot, receiver, params);
        }
    }

    public static boolean invokeBooleanMethod(final int slot,
                                              final Object receiver, final Object... params) {
        if (android.os.Build.VERSION.SDK_INT <= Build.VERSION_CODES.KITKAT_WATCH) {
            return Dalvik.invokeBooleanMethod(slot, receiver, params);
        } else {
            return (boolean) invoke(slot, receiver, params);
        }
    }

    public static byte invokeByteMethod(final int slot, final Object receiver,
                                        final Object... params) {
        if (android.os.Build.VERSION.SDK_INT <= Build.VERSION_CODES.KITKAT_WATCH) {
            return Dalvik.invokeByteMethod(slot, receiver, params);
        } else {
            return (byte) invoke(slot, receiver, params);
        }
    }

    public static short invokeShortMethod(final int slot,
                                          final Object receiver, final Object... params) {
        if (android.os.Build.VERSION.SDK_INT <= Build.VERSION_CODES.KITKAT_WATCH) {
            return Dalvik.invokeShortMethod(slot, receiver, params);
        } else {
            return (short) invoke(slot, receiver, params);
        }
    }

    public static char invokeCharMethod(final int slot, final Object receiver,
                                        final Object... params) {
        if (android.os.Build.VERSION.SDK_INT <= Build.VERSION_CODES.KITKAT_WATCH) {
            return Dalvik.invokeCharMethod(slot, receiver, params);
        } else {
            return (char) invoke(slot, receiver, params);
        }
    }

    public static int invokeIntMethod(final int slot, final Object receiver,
                                      final Object... params) {
        if (android.os.Build.VERSION.SDK_INT <= Build.VERSION_CODES.KITKAT_WATCH) {
            return Dalvik.invokeIntMethod(slot, receiver, params);
        } else {
            return (int) invoke(slot, receiver, params);
        }
    }

    public static long invokeLongMethod(final int slot, final Object receiver,
                                        final Object... params) {
        if (android.os.Build.VERSION.SDK_INT <= Build.VERSION_CODES.KITKAT_WATCH) {
            return Dalvik.invokeLongMethod(slot, receiver, params);
        } else {
            return (long) invoke(slot, receiver, params);
        }
    }

    public static float invokeFloatMethod(final int slot,
                                          final Object receiver, final Object... params) {
        if (android.os.Build.VERSION.SDK_INT <= Build.VERSION_CODES.KITKAT_WATCH) {
            return Dalvik.invokeFloatMethod(slot, receiver, params);
        } else {
            return (float) invoke(slot, receiver, params);
        }
    }

    public static double invokeDoubleMethod(final int slot,
                                            final Object receiver, final Object... params) {
        if (android.os.Build.VERSION.SDK_INT <= Build.VERSION_CODES.KITKAT_WATCH) {
            return Dalvik.invokeDoubleMethod(slot, receiver, params);
        } else {
            return (double) invoke(slot, receiver, params);
        }
    }

    @SuppressWarnings("unchecked")
    public static <T> T invokeObjectMethod(final int slot,
                                           final Object receiver, final Object... params) {
        if (android.os.Build.VERSION.SDK_INT <= Build.VERSION_CODES.KITKAT_WATCH) {
            return (T) Dalvik.invokeObjectMethod(slot, receiver, params);
        } else {
            return (T) invoke(slot, receiver, params);
        }
    }

    /**
     * Returns the result of dynamically invoking this method. Equivalent to
     * {@code receiver.methodName(arg1, arg2, ... , argN)}.
     * <p>
     * <p>
     * If the method is static, the receiver argument is ignored (and may be
     * null).
     * <p>
     * <p>
     * If the invocation completes normally, the return value itself is
     * returned. If the method is declared to return a primitive type, the
     * return value is boxed. If the return type is void, null is returned.
     */
    @SuppressWarnings("unchecked")
    public static <T> T invokeMethod(final int slot, final Object receiver,
                                     final Object... params) {
        return (T) invoke(slot, receiver, params);
    }

    private static final class Dalvik {
        public static native void invokeVoidMethod(final int slot,
                                                   final Object receiver, final Object... params);

        public static native boolean invokeBooleanMethod(final int slot,
                                                         final Object receiver, final Object... params);

        public static native byte invokeByteMethod(final int slot,
                                                   final Object receiver, final Object... params);

        public static native short invokeShortMethod(final int slot,
                                                     final Object receiver, final Object... params);

        public static native char invokeCharMethod(final int slot,
                                                   final Object receiver, final Object... params);

        public static native int invokeIntMethod(final int slot,
                                                 final Object receiver, final Object... params);

        public static native long invokeLongMethod(final int slot,
                                                   final Object receiver, final Object... params);

        public static native float invokeFloatMethod(final int slot,
                                                     final Object receiver, final Object... params);

        public static native double invokeDoubleMethod(final int slot,
                                                       final Object receiver, final Object... params);

        public static native Object invokeObjectMethod(final int slot,
                                                       final Object receiver, final Object... params);
    }
}
