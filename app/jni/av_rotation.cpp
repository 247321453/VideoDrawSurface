//
// Created by user on 2019/4/2.
//
extern "C" {
#include "libavutil/eval.h"
#include "libavutil/display.h"
}

#include "av_rotation.h"

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