/**
 *@file video.c
 *@author Lectem
 *@date 14/06/2015
 */
#include <3ds.h>
#include <libavcodec/avcodec.h>


#include "movie.h"
#include "video.h"


/**
 * open a video stream
 *
 * mv->videoStream must be a valid audio stream
 *
 * @return 0 if opened
 * @return -1 on failure
 */
int video_open_stream(MovieState * mv)
{
    if(mv->videoStream == -1)
        return -1;

    // Get a pointer to the codec context for the video stream
    mv->pCodecCtxOrig=mv->pFormatCtx->streams[mv->videoStream]->codec;
    // Find the decoder for the video stream
    mv->pCodec=avcodec_find_decoder(mv->pCodecCtxOrig->codec_id);
    if(mv->pCodec==NULL) {
        fprintf(stderr, "Unsupported video codec!\n");
        return -1; // Codec not found
    }
    else {
        printf("video decoder : %s - OK\n",mv->pCodec->name);
    }
    // Copy context
    mv->pCodecCtx = avcodec_alloc_context3(mv->pCodec);
    if(avcodec_copy_context(mv->pCodecCtx, mv->pCodecCtxOrig) != 0) {
        fprintf(stderr, "Couldn't copy video codec context");
        return -1; // Error copying codec context
    }

    // Open codec
    if(avcodec_open2(mv->pCodecCtx, mv->pCodec, NULL)<0)
    {
        printf("Could not open codec\n");
        return -1;
    }
    return 0;
}


void video_close_stream(MovieState * mvS)
{
    avcodec_free_context(&mvS->pCodecCtx);
    avcodec_close(mvS->pCodecCtxOrig);
}


void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
    FILE *pFile;
    char szFilename[32];
    int  y;

    // Open file
    sprintf(szFilename, "frame%d.ppm", iFrame);
    pFile=fopen(szFilename, "wb");
    if(pFile==NULL)
        return;

    // Write header
    fprintf(pFile, "P6\n%d %d\n255\n", width, height);

    // Write pixel data
    for(y=0; y<height; y++)
        fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);

    // Close file
    fclose(pFile);
}

void display(AVFrame *pFrame)
{
//    gspWaitForVBlank();
//    gfxSwapBuffers();
    int i,j,c;
    const int width = 400<=pFrame->width?400:pFrame->width;
    const int height = 240<=pFrame->height?240:pFrame->height;
    if(gfxGetScreenFormat(GFX_TOP) == GSP_BGR8_OES)
    {

        u8* const src = pFrame->data[0];
        u8* const fbuffer = gfxGetFramebuffer(GFX_TOP,GFX_LEFT,0,0);

        for (i = 0; i < width; ++i)
        {
            for (j = 0; j < height; ++j)
            {
                for (c = 0; c < 3; ++c)
                {
                    fbuffer[3 * 240 * i + (239 - j) * 3 + c] = src[1024 * 3 * j + i * 3 + c];
                }
            }
        }
    }
    else if(gfxGetScreenFormat(GFX_TOP) == GSP_RGBA8_OES)
    {

        u32* const src = (u32 *) pFrame->data[0];
        u32* const fbuffer = (u32 *) gfxGetFramebuffer(GFX_TOP,GFX_LEFT,0,0);
        for (i = 0; i < width; ++i)
        {
            for (j = 0; j < height; ++j)
            {
                fbuffer[240 * i + (239 - j)] = src[1024 * j + i];
            }
        }
    }
    else
    {
        printf("format not supported\n");
    }
}

void displayGPU()
{

}
