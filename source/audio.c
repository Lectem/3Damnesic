/**
 *@file audio.cpp
 *@author Lectem
 *@date 14/06/2015
 */
#include <movie.h>
#include <stdio.h>
#include <libavcodec/avcodec.h>
#include "audio.h"

/**
 * open an audio stream
 *
 * mv->audioStream must be a valid audio stream
 *
 * @return 0 if opened
 * @return -1 on failure
 */
int audio_open_stream(MovieState * mv)
{
    if(mv->audioStream == -1)
        return -1;

    mv->aCodecCtxOrig=mv->pFormatCtx->streams[mv->audioStream]->codec;

    mv->aCodec = avcodec_find_decoder(mv->aCodecCtxOrig->codec_id);

    if(!mv->aCodec) {
        fprintf(stderr, "Unsupported audio codec!\n");
        return -1;
    }
    else
    {
        printf("audio decoder : %s - OK\n",mv->aCodec->name);
    }

    // Copy context
    mv->aCodecCtx = avcodec_alloc_context3(mv->aCodec);
    if(avcodec_copy_context(mv->aCodecCtx, mv->aCodecCtxOrig) != 0) {
        fprintf(stderr, "Couldn't copy audio codec context");
        return -1; // Error copying codec context
    }

    if( avcodec_open2(mv->aCodecCtx, mv->aCodec, NULL) < 0) {
        fprintf(stderr, "Couldn't open audio codec context");
        return -1; // Error copying codec context
    }

    return 0;
}


void audio_close_stream(MovieState * mvS)
{
    avcodec_free_context(&mvS->aCodecCtx);
    avcodec_close(mvS->aCodecCtxOrig);
}
