LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE :=  libijkffmpeg
LOCAL_SRC_FILES := prebuilt/${TARGET_ARCH_ABI}/libijkffmpeg.so
include $(PREBUILT_SHARED_LIBRARY)

############
include $(CLEAR_VARS)

LOCAL_MODULE := kkplayer


LOCAL_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_SRC_FILES := main.cpp \
    video_player.cpp \
    av_rotation.cpp

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS

LOCAL_LDLIBS := -llog -landroid

LOCAL_STATIC_LIBRARIES := libijkffmpeg

include $(BUILD_SHARED_LIBRARY)
