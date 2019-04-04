LOCAL_PATH := $(call my-dir)

LOCAL_LIBYUV_DIR := $(LOCAL_PATH)/../../libyuvutil/libyuv/jni

############
include $(CLEAR_VARS)
LOCAL_MODULE :=  libijkffmpeg
LOCAL_SRC_FILES := prebuilt/${TARGET_ARCH_ABI}/libijkffmpeg.so
include $(PREBUILT_SHARED_LIBRARY)
#############
#################libyuv
include $(CLEAR_VARS)

LOCAL_CPP_EXTENSION := .cc

LOCAL_SRC_FILES := \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/compare.cc           \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/compare_common.cc    \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/compare_gcc.cc       \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/compare_mmi.cc       \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/compare_msa.cc       \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/compare_neon.cc      \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/compare_neon64.cc    \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/convert.cc           \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/convert_argb.cc      \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/convert_from.cc      \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/convert_from_argb.cc \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/convert_to_argb.cc   \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/convert_to_i420.cc   \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/cpu_id.cc            \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/planar_functions.cc  \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/rotate.cc            \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/rotate_any.cc        \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/rotate_argb.cc       \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/rotate_common.cc     \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/rotate_gcc.cc        \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/rotate_mmi.cc        \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/rotate_msa.cc        \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/rotate_neon.cc       \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/rotate_neon64.cc     \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/row_any.cc           \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/row_common.cc        \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/row_gcc.cc           \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/row_mmi.cc           \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/row_msa.cc           \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/row_neon.cc          \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/row_neon64.cc        \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/scale.cc             \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/scale_any.cc         \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/scale_argb.cc        \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/scale_common.cc      \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/scale_gcc.cc         \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/scale_mmi.cc         \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/scale_msa.cc         \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/scale_neon.cc        \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/scale_neon64.cc      \
    $(LOCAL_LIBYUV_DIR)/libyuv/source/video_common.cc

common_CFLAGS := -Wall -fexceptions

LOCAL_CFLAGS += $(common_CFLAGS)
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_LIBYUV_DIR)/libyuv/include
LOCAL_C_INCLUDES += $(LOCAL_LIBYUV_DIR)/libyuv/include
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_LIBYUV_DIR)/libyuv/include

LOCAL_MODULE := libyuv_static
LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)

############
include $(CLEAR_VARS)

LOCAL_MODULE := kkplayer

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_C_INCLUDES += $(LOCAL_LIBYUV_DIR)
LOCAL_C_INCLUDES += $(LOCAL_LIBYUV_DIR)/libyuv/include

LOCAL_SRC_FILES := main.cpp \
    video_player.cpp \
    util.cpp \
	$(LOCAL_LIBYUV_DIR)/yuv_util.cpp

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS

LOCAL_LDLIBS := -llog -landroid

LOCAL_SHARED_LIBRARIES := libijkffmpeg

LOCAL_STATIC_LIBRARIES := libyuv_static

include $(BUILD_SHARED_LIBRARY)
