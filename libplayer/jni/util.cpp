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

int av_frame_to_jpeg(AVFrame* frame,uint8_t * jpeg, int q){
    //TODO
    return 0;
}

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
av_yuv420p_to_nv21(AVFrame *frame, int width, int height, uint8_t *nv21Data) {
    int dst_stride_y = width, src_stride_y = width;
    int dst_stride_vu = width;

    size_t y_size = static_cast<size_t>(src_stride_y * height);

    int ret = libyuv::I420ToNV21(
            frame->data[0], frame->linesize[0],
            frame->data[1], frame->linesize[1],
            frame->data[2], frame->linesize[2],
            nv21Data, dst_stride_y,
            nv21Data + y_size, dst_stride_vu,
            width, height);

    return ret;
}

long getCurTime() {
    return (long) (std::clock() / 1000);
}

int av_frame_scale(AVFrame *src, AVFrame *dst, int dst_width, int dst_height) {
    dst->width = dst_width;
    dst->height = dst_height;
    return libyuv::I420Scale(
            src->data[0], src->linesize[0],
            src->data[1], src->linesize[1],
            src->data[2], src->linesize[2],
            src->width, src->height,
            dst->data[0], dst->linesize[0],
            dst->data[1], dst->linesize[1],
            dst->data[2], dst->linesize[2],
            dst_width, dst_height, libyuv::kFilterBox
    );
}
/**
 *
 * @param src
 * @param rotation
 * @param left
 * @param top
 * @param dst_width 旋转之前的宽
 * @param dst_height 旋转之前的高
 * @param dst
 * @return
 */
int av_frame_rotate_crop(AVFrame *src, int rotation,
                         int left, int top, int dst_width, int dst_height, AVFrame *dst) {
    libyuv::RotationMode mode;
    if (rotation == ROTATION_90) {
        mode = libyuv::kRotate90;
    } else if (rotation == ROTATION_180) {
        mode = libyuv::kRotate180;
    } else if (rotation == ROTATION_270) {
        mode = libyuv::kRotate270;
    } else {
        mode = libyuv::kRotate0;
    }
    int src_width = src->width;
//    int src_height = src->height;
    int half_width = src_width >> 1;

    int y_start = src_width * top + left;
    int u_start = half_width * (top >> 1) + (left >> 1);
    int v_start = u_start;

    const uint8_t *src_y = src->data[0] + y_start;
    const uint8_t *src_u = src->data[1] + u_start;
    const uint8_t *src_v = src->data[2] + v_start;

    if (mode == libyuv::kRotate90 || mode == libyuv::kRotate270) {
        dst->width = dst_height;
        dst->height = dst_width;
        dst->linesize[0] = dst_height;
        dst->linesize[1] = dst_height >> 1;
        dst->linesize[2] = dst_height >> 1;
    } else {
        dst->width = dst_width;
        dst->height = dst_height;
        dst->linesize[0] = dst_width;
        dst->linesize[1] = dst_width >> 1;
        dst->linesize[2] = dst_width >> 1;
    }

    if (mode == libyuv::kRotate0) {
        dst->data[0] = src->data[0];
        dst->data[1] = src->data[1];
        dst->data[2] = src->data[2];
        return 0;
    }
    int ret = libyuv::I420Rotate(
            src_y, src->linesize[0],
            src_u, src->linesize[1],
            src_v, src->linesize[2],
            dst->data[0], dst->linesize[0],
            dst->data[1], dst->linesize[1],
            dst->data[2], dst->linesize[2],
            dst_width, dst_height, mode);
    if (ret == 0) {
        dst->format = src->format;
        dst->pts = src->pts;
        dst->pkt_pts = src->pkt_pts;
        dst->pkt_dts = src->pkt_dts;
        dst->key_frame = src->key_frame;
    }
    return ret;
}

int av_frame_rotate(AVFrame *src, int rotation, AVFrame *dst) {
    libyuv::RotationMode mode;
    if (rotation == ROTATION_90) {
        mode = libyuv::kRotate90;
    } else if (rotation == ROTATION_180) {
        mode = libyuv::kRotate180;
    } else if (rotation == ROTATION_270) {
        mode = libyuv::kRotate270;
    } else {
        mode = libyuv::kRotate0;
    }
    int width = src->width;
    int height = src->height;

    int ret;
    if (mode == libyuv::kRotate90 || mode == libyuv::kRotate270) {
        ret = libyuv::I420Rotate((const uint8_t *) src->data[0], src->linesize[0],
                                 (const uint8_t *) src->data[1], src->linesize[1],
                                 (const uint8_t *) src->data[2], src->linesize[2],
                                 dst->data[0], height,
                                 dst->data[1], height >> 1,
                                 dst->data[2], height >> 1,
                                 width, height, mode);
        dst->width = height;
        dst->height = width;
        dst->linesize[0] = height;
        dst->linesize[1] = height >> 1;
        dst->linesize[2] = height >> 1;
    } else {
        if (mode == libyuv::kRotate0) {
            dst->data[0] = src->data[0];
            dst->data[1] = src->data[1];
            dst->data[2] = src->data[2];
            ret = 0;
        } else {
            ret = libyuv::I420Rotate((const uint8_t *) src->data[0], src->linesize[0],
                                     (const uint8_t *) src->data[1], src->linesize[1],
                                     (const uint8_t *) src->data[2], src->linesize[2],
                                     dst->data[0], width,
                                     dst->data[1], width >> 1,
                                     dst->data[2], width >> 1,
                                     width, height, mode);
        }
        dst->width = width;
        dst->height = height;
        dst->linesize[0] = width;
        dst->linesize[1] = width >> 1;
        dst->linesize[2] = width >> 1;
    }

    dst->format = src->format;
    dst->pts = src->pts;
    dst->pkt_pts = src->pkt_pts;
    dst->pkt_dts = src->pkt_dts;
    dst->key_frame = src->key_frame;
    return ret;
}