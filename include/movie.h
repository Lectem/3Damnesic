/**
 *@file movie.h
 *@author Lectem
 *@date 14/06/2015
 */
#pragma once
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include "3ds.h"


typedef enum{
    CONVERT_COL_SWSCALE =0,
    CONVERT_COL_Y2R     =1,
}CONVERT_COLOR_METHOD;

typedef struct MovieState {
    AVFormatContext   * pFormatCtx ;
    int                 videoStream,audioStream;

    // video codec
    AVCodecContext    *pCodecCtxOrig ;
    AVCodecContext    *pCodecCtx ;
    AVCodec           *pCodec ;

    // audio codec
    AVCodecContext    *aCodecCtxOrig;
    AVCodecContext    *aCodecCtx;
    AVCodec           *aCodec ;

    // video frame
    AVFrame           *pFrame ;
    AVFrame           *outFrame;// 1024x1024 texture
    u16               out_bpp;
    AVPacket          packet;
    struct SwsContext *sws_ctx ;
    CONVERT_COLOR_METHOD convertColorMethod;
    Y2R_ConversionParams params;
    Handle end_event;
    bool renderGpu;
} MovieState;


int  setup(MovieState * mv,char * filename);
void tearup(MovieState *mv);
