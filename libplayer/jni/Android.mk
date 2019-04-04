LOCAL_PATH := $(call my-dir)

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
    libyuv/source/compare.cc           \
    libyuv/source/compare_common.cc    \
    libyuv/source/compare_gcc.cc       \
    libyuv/source/compare_mmi.cc       \
    libyuv/source/compare_msa.cc       \
    libyuv/source/compare_neon.cc      \
    libyuv/source/compare_neon64.cc    \
    libyuv/source/convert.cc           \
    libyuv/source/convert_argb.cc      \
    libyuv/source/convert_from.cc      \
    libyuv/source/convert_from_argb.cc \
    libyuv/source/convert_to_argb.cc   \
    libyuv/source/convert_to_i420.cc   \
    libyuv/source/cpu_id.cc            \
    libyuv/source/planar_functions.cc  \
    libyuv/source/rotate.cc            \
    libyuv/source/rotate_any.cc        \
    libyuv/source/rotate_argb.cc       \
    libyuv/source/rotate_common.cc     \
    libyuv/source/rotate_gcc.cc        \
    libyuv/source/rotate_mmi.cc        \
    libyuv/source/rotate_msa.cc        \
    libyuv/source/rotate_neon.cc       \
    libyuv/source/rotate_neon64.cc     \
    libyuv/source/row_any.cc           \
    libyuv/source/row_common.cc        \
    libyuv/source/row_gcc.cc           \
    libyuv/source/row_mmi.cc           \
    libyuv/source/row_msa.cc           \
    libyuv/source/row_neon.cc          \
    libyuv/source/row_neon64.cc        \
    libyuv/source/scale.cc             \
    libyuv/source/scale_any.cc         \
    libyuv/source/scale_argb.cc        \
    libyuv/source/scale_common.cc      \
    libyuv/source/scale_gcc.cc         \
    libyuv/source/scale_mmi.cc         \
    libyuv/source/scale_msa.cc         \
    libyuv/source/scale_neon.cc        \
    libyuv/source/scale_neon64.cc      \
    libyuv/source/video_common.cc

common_CFLAGS := -Wall -fexceptions

LOCAL_CFLAGS += $(common_CFLAGS)
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/libyuv/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/libyuv/include
LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/libyuv/include

LOCAL_MODULE := libyuv
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

############
include $(CLEAR_VARS)

LOCAL_MODULE := kkplayer

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include
LOCAL_C_INCLUDES += $(LOCAL_PATH)/libyuv/include

LOCAL_SRC_FILES := main.cpp \
    video_player.cpp \
    util.cpp

LOCAL_CFLAGS := -D__STDC_CONSTANT_MACROS

LOCAL_LDLIBS := -llog -landroid

LOCAL_SHARED_LIBRARIES := libijkffmpeg \
    libyuv

include $(BUILD_SHARED_LIBRARY)
