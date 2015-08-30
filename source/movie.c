/**
 *@file movie.c
 *@author Lectem
 *@date 14/06/2015
 */
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <assert.h>
#include <color_converter.h>
#include "audio.h"
#include "video.h"
#include "movie.h"

static unsigned int next_pow2(unsigned int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v+1;
}


int setup(MovieState * mv,char * filename)
{


    memset(mv,0,sizeof(*mv));

    mv->videoStream = -1;
    mv->audioStream = -1;
    mv->renderGpu =true;

    mv->convertColorMethod=CONVERT_COL_Y2R; // TODO : check if the format is supported

    int i;

    // Open video file
    int averr=avformat_open_input(&mv->pFormatCtx, filename, NULL, NULL);
    if(averr!=0)
    {
        char buf[100];
        av_strerror(averr,buf,100);
        printf("Couldn't open file : %s\n",buf);
        return -1;
    }

    // Retrieve stream information
    if(avformat_find_stream_info(mv->pFormatCtx, NULL)<0)
    {
        printf("Couldn't find stream information\n");
        return -1;
    }

    printf("dumping info...\n");
    // Dump information about file onto standard error
    av_dump_format(mv->pFormatCtx, 0, filename, 0);
    printf("\n");


    // TODO Use av_find_best_stream ?
    // Find the first video and audio streams
    mv->videoStream=-1;
    for(i=0; i<mv->pFormatCtx->nb_streams; i++)
    {
        if(mv->pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO && mv->videoStream < 0) {
            mv->videoStream=i;
        }
        if(mv->pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO && mv->audioStream < 0) {
            mv->audioStream=i;
        }
    }
    if(mv->videoStream == -1)
    {
        printf("Didn't find a video stream\n");
                return -1;
    }
    if(mv->audioStream == -1)
    {
        printf("Didn't find an audio stream\n");
                return -1;
    }

    printf("video stream %d\n",mv->videoStream);
    printf("audio stream %d\n\n",mv->audioStream);

    if(video_open_stream(mv))return -1;
    if(audio_open_stream(mv))return -1;

    // Allocate video frame
    mv->pFrame=av_frame_alloc();
    if(mv->pFrame==NULL)
    {
        printf("Couldnt alloc pFrame\n");
        return -1;
    }

    // Allocate an AVFrame structure
    mv->outFrame =av_frame_alloc();
    if(mv->outFrame ==NULL)
    {
        printf("Couldnt alloc outFrame\n");
        return -1;
    }

    /**
     * Allocate a texture of minimal size to contain the decoded video
     * We cannot use 1024x1024 if width <= 512 because of the gap size limit in y2r
     */
    mv->outFrame->width=next_pow2(mv->pCodecCtx->width);
    mv->outFrame->height=next_pow2(mv->pCodecCtx->height);
    if(mv->renderGpu)                                    mv->out_bpp=4;
    else if(gfxGetScreenFormat(GFX_TOP) == GSP_BGR8_OES )mv->out_bpp=3;
    else if(gfxGetScreenFormat(GFX_TOP) == GSP_RGBA8_OES)mv->out_bpp=4;
    mv->outFrame->linesize[0]=mv->outFrame->width*mv->out_bpp;
    mv->outFrame->data[0] = linearMemAlign(mv->outFrame->width*mv->outFrame->height*mv->out_bpp,0x80);//RGBA next_pow2(width) x 1024 texture


    if(initColorConverter(mv)<0)
    {
        exitColorConvert(mv);
        return -1;
    }

    /**
     * TODO Clean up when fail
     */
    return 0;
}


void tearup(MovieState *mv)
{
    av_free_packet(&mv->packet);
    exitColorConvert(mv);

    // Free the RGB image
    if(mv->outFrame->data[0])linearFree(mv->outFrame->data[0]);
    av_frame_free(&mv->outFrame);

    // Free the YUV frame
    av_frame_free(&mv->pFrame);

    // Close the codecs
    audio_close_stream(mv);
    video_close_stream(mv);

    // Close the video file
    avformat_close_input(&mv->pFormatCtx);
}