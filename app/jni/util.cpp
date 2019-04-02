//
// Created by user on 2019/4/2.
//
extern "C" {
#include "libavutil/eval.h"
#include "libavutil/display.h"
}

#include <ctime>
#include "libyuv.h"
#include "util.h"
#include "debug.h"

int av_get_rotation(AVStream *st) {
    AVDictionaryEntry *rotate_tag = av_dict_get(st->metadata, "rotate", NULL, 0);
    uint8_t *dmtx = av_stream_get_side_data(st, AV_PKT_DATA_DISPLAYMATRIX, NULL);
    double theta = 0;

    if (rotate_tag && *rotate_tag->value && strcmp(rotate_tag->value, "0")) {
        char *tail;
        theta = av_strtod(rotate_tag->value, &tail);
        if (*tail)
            theta = 0;
    }
    if (dmtx && !theta) {
        theta = -av_display_rotation_get((int32_t *) dmtx);
    }
    theta -= 360 * floor(theta / 360 + 0.9 / 360);

//    if (fabs(theta - 90 * round(theta / 90)) > 2) {
//        av_log(NULL, AV_LOG_WARNING, "Odd mRotation angle.\n"
//                                     "If you want to help, upload a sample "
//                                     "of this file to ftp://upload.ffmpeg.org/incoming/ "
//                                     "and contact the ffmpeg-devel mailing list. (ffmpeg-devel@ffmpeg.org)");
//        LOGE("theta:2");
//    } else {
//        LOGD("theta:1");
//    }
    if (fabs(theta - 90) < 1.0) {
        return ROTATION_90;
    } else if (fabs(theta - 180) < 1.0 || fabs(theta + 180) < 1.0) {
        return ROTATION_180;
    } else if (fabs(theta - 270) < 1.0 || fabs(theta + 90) < 1.0) {
        return ROTATION_270;
    } else {
        return ROTATION_0;
    }
}


int
i420_to_nv21(uint8_t *i420Data, int width, int height, uint8_t *nv21Data) {
    int dst_stride_y = width, src_stride_y = width;
    int dst_stride_vu = width;
    int src_stride_u = width >> 1;
    int src_stride_v = src_stride_u;

    size_t y_size = static_cast<size_t>(src_stride_y * height);
    size_t u_size = static_cast<size_t>(src_stride_u * (height >> 1));

    int ret = libyuv::I420ToNV21(
            i420Data, src_stride_y,
            i420Data + y_size, src_stride_u,
            i420Data + y_size + u_size, src_stride_v,
            nv21Data, dst_stride_y,
            nv21Data + y_size, dst_stride_vu,
            width, height);

    return ret;
}

int
nv21_to_rbga(uint8_t *nv21Data, int width, int height, uint8_t *argbData) {

    size_t ySize = static_cast<size_t>(width * height);

    //I420ToABGR
    int ret = libyuv::NV12ToABGR(
            nv21Data, width,
            nv21Data + ySize, width,
            argbData, width * 4,
            width, height);
    return ret;
}

int
i420_to_rbga(uint8_t *i420Data, int width, int height, uint8_t *argbData) {

    int src_y_stride = width;
    int src_u_stride = width >> 1;
    int src_v_stride = src_u_stride;
    size_t ySize = static_cast<size_t>(width * height);
    size_t uSize = static_cast<size_t>(src_u_stride * (height >> 1));

    int ret = libyuv::I420ToABGR(
            i420Data, src_y_stride,
            i420Data + ySize, src_u_stride,
            i420Data + ySize + uSize, src_v_stride,
            argbData, width * 4,
            width, height);
    return ret;
}

long getCurTime(){
    return (long) (std::clock() / 1000);
}

int
convert(AVFrame *frame, int rotation, int width, int height, uint8_t *nv21Data,
        uint8_t *rgbaData) {
    //ALOGD("convert start");


    int ret;
    uint8_t *src_i420_y_data = frame->data[0];
    uint8_t *src_i420_u_data = frame->data[1];
    uint8_t *src_i420_v_data = frame->data[2];
    int src_stride_y = width;
    int src_stride_u = width >> 1;
    int src_stride_v = src_stride_u;

    uint8_t *i420Data = (uint8_t *) malloc(sizeof(uint8_t) * width * height * 3 / 2);
    size_t y_size = static_cast<size_t>(width * height);
    size_t u_size = static_cast<size_t>((width >> 1) * (height >> 1));
    long cur, time = getCurTime();

    libyuv::RotationMode  mode;
    if (rotation == ROTATION_90) {
        mode = libyuv::kRotate90;
    } else if (rotation == ROTATION_180) {
        mode = libyuv::kRotate180;
    } else if (rotation == ROTATION_270) {
        mode = libyuv::kRotate270;
    } else {
        mode = libyuv::kRotate0;
    }
    if (mode == libyuv::kRotate90 || mode == libyuv::kRotate270) {
        ret = libyuv::I420Rotate((const uint8_t *) src_i420_y_data, src_stride_y,
                                 (const uint8_t *) src_i420_u_data, src_stride_u,
                                 (const uint8_t *) src_i420_v_data, src_stride_v,
                                 i420Data, height,
                                 i420Data + y_size, height >> 1,
                                 i420Data + y_size + u_size, height >> 1,
                                 width, height, mode);
        int tmp = height;
        height = width;
        width = tmp;
    } else {
        ret = libyuv::I420Rotate((const uint8_t *) src_i420_y_data, src_stride_y,
                                 (const uint8_t *) src_i420_u_data, src_stride_u,
                                 (const uint8_t *) src_i420_v_data, src_stride_v,
                                 i420Data, width,
                                 i420Data + y_size, width >> 1,
                                 i420Data + y_size + u_size, width >> 1,
                                 width, height, mode);
    }
    cur = getCurTime();
    ALOGD("yuv rotate use time %ld", (cur - time));
    time = cur;

    if (ret != 0) {
        ALOGW("i420 rotate error:%d", ret);
        return ret;
    }
    if(nv21Data != NULL) {
        ret = i420_to_nv21(i420Data, width, height, nv21Data);
        if (ret != 0) {
            ALOGW("i420_to_nv21 error:%d", ret);
            return ret;
        }
        cur = getCurTime();
        ALOGD("yuv nv21 use time %ld", (cur - time));
        time = cur;
//        if(rgbaData != NULL) {
//            ret = nv21_to_rbga(nv21Data, width, height, rgbaData);
//            if (ret != 0) {
//                ALOGW("nv21_to_rbga error:%d", ret);
//            }
//        }
//        cur = getCurTime();
//        ALOGD("yuv nv21 use time %ld", (cur - time));
    }

    if (rgbaData != NULL) {
        ret = i420_to_rbga(i420Data, width, height, rgbaData);
        if (ret != 0) {
            ALOGW("i420_to_rbga error:%d", ret);
        }
    }
    cur = getCurTime();
    ALOGD("yuv rgba use time %ld", (cur - time));
    return ret;
}

void av_frame_rotate_90(AVFrame *src, AVFrame *des) {
    int n = 0;
    int hw = src->width >> 1;
    int hh = src->height >> 1;
    int size = src->width * src->height;
    int hsize = size >> 2;

    int i,j,pos = 0;
    //copy y
    for (j = 0; j < src->width; j++) {
        pos = size;
        for (i = src->height - 1; i >= 0; i--) {
            pos -= src->width;
            des->data[0][n++] = src->data[0][pos + j];
        }
    }
    //copy uv
    n = 0;
    for (j = 0; j < hw; j++) {
        pos = hsize;
        for (i = hh - 1; i >= 0; i--) {
            pos -= hw;
            des->data[1][n] = src->data[1][pos + j];
            des->data[2][n] = src->data[2][pos + j];
            n++;
        }
    }

    des->linesize[0] = src->height;
    des->linesize[1] = src->height >> 1;
    des->linesize[2] = src->height >> 1;
    des->height = src->width;
    des->width = src->height;
    des->format = src->format;

    des->pts = src->pts;
    des->pkt_pts = src->pkt_pts;
    des->pkt_dts = src->pkt_dts;

    des->key_frame = src->key_frame;
}

void av_frame_rotate_180(AVFrame *src, AVFrame *des) {
    int n = 0, i = 0, j = 0;
    int hw = src->width >> 1;
    int hh = src->height >> 1;
    int pos = src->width * src->height;

    for (i = 0; i < src->height; i++) {
        pos -= src->width;
        for (j = 0; j < src->width; j++) {
            des->data[0][n++] = src->data[0][pos + j];
        }
    }

    n = 0;
    pos = src->width * src->height >> 2;

    for (i = 0; i < hh; i++) {
        pos -= hw;
        for (j = 0; j < hw; j++) {

            des->data[1][n] = src->data[1][pos + j];
            des->data[2][n] = src->data[2][pos + j];
            n++;
        }
    }

    des->linesize[0] = src->width;
    des->linesize[1] = src->width >> 1;
    des->linesize[2] = src->width >> 1;

    des->width = src->width;
    des->height = src->height;
    des->format = src->format;

    des->pts = src->pts;
    des->pkt_pts = src->pkt_pts;
    des->pkt_dts = src->pkt_dts;

    des->key_frame = src->key_frame;
}

void av_frame_rotate_270(AVFrame *src, AVFrame *des) {
    int n = 0, i = 0, j = 0;
    int hw = src->width >> 1;
    int hh = src->height >> 1;
    int pos = 0;

    for (i = src->width - 1; i >= 0; i--) {
        pos = 0;
        for (j = 0; j < src->height; j++) {
            des->data[0][n++] = src->data[0][pos + i];
            pos += src->width;
        }
    }

    n = 0;
    for (i = hw - 1; i >= 0; i--) {
        pos = 0;
        for (j = 0; j < hh; j++) {
            des->data[1][n] = src->data[1][pos + i];
            des->data[2][n] = src->data[2][pos + i];
            pos += hw;
            n++;
        }
    }

    des->linesize[0] = src->height;
    des->linesize[1] = src->height >> 1;
    des->linesize[2] = src->height >> 1;

    des->width = src->height;
    des->height = src->width;
    des->format = src->format;

    des->pts = src->pts;
    des->pkt_pts = src->pkt_pts;
    des->pkt_dts = src->pkt_dts;

    des->key_frame = src->key_frame;
}