/**
 *@file color_converter.c
 *@author Lectem
 *@date 18/06/2015
 */
#include <libswscale/swscale.h>
#include "color_converter.h"
#include <3ds.h>

#include <assert.h>
#include <3ds/services/y2r.h>

static inline double u64_to_double(u64 value)
{
    return (((double) (u32) (value >> 32)) * 0x100000000ULL + (u32) value);
}

int initColorConverterSwscale(MovieState *mvS)
{
    enum AVPixelFormat out_fmt = AV_PIX_FMT_BGR24;
    if (gfxGetScreenFormat(GFX_TOP) == GSP_RGBA8_OES) out_fmt = AV_PIX_FMT_BGRA;
    // initialize SWS context for software scaling
    mvS->sws_ctx = sws_getContext(mvS->pCodecCtx->width, mvS->pCodecCtx->height, mvS->pCodecCtx->pix_fmt,
                                  mvS->pCodecCtx->width, mvS->pCodecCtx->height, out_fmt,
                                  0,//SWS_BILINEAR,//TODO perf comparison
                                  NULL,
                                  NULL,
                                  NULL
    );


    if (mvS->sws_ctx == NULL)
    {
        printf("Couldnt initialize sws\n");
        return -1;
    }
    return 0;
}


int exitColorConvertSwscale(MovieState *mvS)
{
    sws_freeContext(mvS->sws_ctx);
    return 0;
}


int colorConvertSwscale(MovieState *mvS)
{
    // Convert the image from its native format to RGB
    sws_scale(mvS->sws_ctx, (uint8_t const *const *) mvS->pFrame->data,
              mvS->pFrame->linesize, 0, mvS->pCodecCtx->height,
              mvS->outFrame->data, mvS->outFrame->linesize);
    return 0;
}

#define CHECKRES() do{if(res != 0){printf("error %lx L%d %s\n",(u32)res,__LINE__,osStrError(res));return -1;}}while(0);

int ffmpegPixelFormatToY2R(enum AVPixelFormat pix_fmt)
{
    switch (pix_fmt)
    {
        case AV_PIX_FMT_YUV420P     :
            return INPUT_YUV420_INDIV_8;
        case AV_PIX_FMT_YUV422P     :
            return INPUT_YUV422_INDIV_8;
        case AV_PIX_FMT_YUV420P16LE :
            return INPUT_YUV420_INDIV_16;
        case AV_PIX_FMT_YUV422P16LE :
            return INPUT_YUV422_INDIV_16;
        case AV_PIX_FMT_YUYV422     :
            return INPUT_YUV422_BATCH;
        default:
            printf("unknown format %d, using INPUT_YUV420_INDIV_8\n", pix_fmt);
            return INPUT_YUV420_INDIV_8;
    }
}

int initColorConverterY2r(MovieState *mvS)
{
    Result res = 0;
    res = y2rInit();
    CHECKRES();
    res = Y2RU_StopConversion();
    CHECKRES();
    bool is_busy = 0;
    int tries = 0;
    do
    {
        svcSleepThread(100000ull);
        res = Y2RU_StopConversion();
        CHECKRES();
        res = Y2RU_IsBusyConversion(&is_busy);
        CHECKRES();
        tries += 1;
//        printf("is_busy %d\n",is_busy);
    } while (is_busy && tries < 100);

    mvS->params.input_format = ffmpegPixelFormatToY2R(mvS->pCodecCtx->pix_fmt);
    if (mvS->out_bpp == 3) mvS->params.output_format = OUTPUT_RGB_24;
    else if (mvS->out_bpp == 4) mvS->params.output_format = OUTPUT_RGB_32;
    mvS->params.rotation = ROTATION_NONE;
    mvS->params.block_alignment = BLOCK_8_BY_8;
    mvS->params.input_line_width = mvS->pCodecCtx->width;
    mvS->params.input_lines = mvS->pCodecCtx->height;
    if (mvS->params.input_lines % 8)
    {
        mvS->params.input_lines += 8 - mvS->params.input_lines % 8;
        printf("Height not multiple of 8, cropping to %dpx\n", mvS->params.input_lines);
    }
    mvS->params.standard_coefficient = COEFFICIENT_ITU_R_BT_601;//TODO : detect
    mvS->params.unused = 0;
    mvS->params.alpha = 0xFF;

    res = Y2RU_SetConversionParams(&mvS->params);
    CHECKRES();

    res = Y2RU_SetTransferEndInterrupt(true);
    CHECKRES();
    mvS->end_event = 0;
    res = Y2RU_GetTransferEndEvent(&mvS->end_event);
    CHECKRES();


    return 0;
}


int colorConvertY2R(MovieState *mvS)
{
    Result res;

    const u16 img_w = mvS->params.input_line_width;
    const u16 img_h = mvS->params.input_lines;
    const u32 img_size = img_w * img_h;

    size_t src_Y_size = 0;
    size_t src_UV_size = 0;
    switch (mvS->params.input_format)
    {
        case INPUT_YUV422_INDIV_8:
            src_Y_size = img_size;
            src_UV_size = img_size / 2;
            break;
        case INPUT_YUV420_INDIV_8:
            src_Y_size = img_size;
            src_UV_size = img_size / 4;
            break;
        case INPUT_YUV422_INDIV_16:
            src_Y_size = img_size * 2;
            src_UV_size = img_size / 2 * 2;
            break;
        case INPUT_YUV420_INDIV_16:
            src_Y_size = img_size * 2;
            src_UV_size = img_size / 4 * 2;
            break;
        case INPUT_YUV422_BATCH:
            src_Y_size = img_size * 2;
            src_UV_size = img_size * 2;
            break;
    }
    if (mvS->params.input_format == INPUT_YUV422_BATCH)
    {
        //TODO : test it
        assert(mvS->pFrame->linesize[0] >= src_Y_size);
        res = Y2RU_SetSendingYUYV(mvS->pFrame->data[0], src_Y_size, img_w, mvS->pFrame->linesize[0] - src_Y_size);
    }
    else
    {
        const u8 *src_Y = mvS->pFrame->data[0];
        const u8 *src_U = mvS->pFrame->data[1];
        const u8 *src_V = mvS->pFrame->data[2];
        const u16 src_Y_padding = mvS->pFrame->linesize[0] - img_w;
        const u16 src_UV_padding = mvS->pFrame->linesize[1] - (img_w >> 1);

        res = Y2RU_SetSendingY(src_Y, src_Y_size, img_w, src_Y_padding);
        CHECKRES();
        res = Y2RU_SetSendingU(src_U, src_UV_size, img_w >> 1, src_UV_padding);
        CHECKRES();
        res = Y2RU_SetSendingV(src_V, src_UV_size, img_w >> 1, src_UV_padding);
        CHECKRES();
    }

    const u16 out_bpp = mvS->out_bpp;
    size_t rgb_size = img_size * out_bpp;
    s16 gap = (mvS->outFrame->width - img_w) * 8 * out_bpp;
    if (mvS->outFrame->width * 8 * out_bpp >= 0x8000)
    {
        printf("This image is too wide for y2r.\n");
        return 1;
        //TODO : check at setup
    }
    res = Y2RU_SetReceiving(mvS->outFrame->data[0], rgb_size, img_w * 8 * out_bpp, gap);
    CHECKRES();
    res = Y2RU_StartConversion();
    CHECKRES();
    u64 beforeTick = svcGetSystemTick();
    res = svcWaitSynchronization(mvS->end_event, 1000 * 1000 * 10);
    u64 afterTick = svcGetSystemTick();

#define TICKS_PER_USEC 268.123480
#define TICKS_PER_MSEC 268123.480
    if (res)
    {
        printf("outdim %d unitsize %d gap %d\n", mvS->outFrame->width * 8 * out_bpp, img_w * 8 * out_bpp, gap);
        printf("svcWaitSynchronization %lx\n", res);
    }
//    else printf("waited %lf\n",u64_to_double(afterTick-beforeTick)/TICKS_PER_USEC);

    return 0;
}

int exitColorConvertY2R(MovieState *mvS)
{
    Result res = 0;
    bool is_busy = 0;

    Y2RU_StopConversion();
    Y2RU_IsBusyConversion(&is_busy);
    y2rExit();
    return 0;
}


int initColorConverter(MovieState *mvS)
{
    switch (mvS->convertColorMethod)
    {
        default:
        case CONVERT_COL_SWSCALE:
            return initColorConverterSwscale(mvS);
        case CONVERT_COL_Y2R:
            return initColorConverterY2r(mvS);
    }
}


int colorConvert(MovieState *mvS)
{
    switch (mvS->convertColorMethod)
    {
        default:
        case CONVERT_COL_SWSCALE:
            return colorConvertSwscale(mvS);
        case CONVERT_COL_Y2R:
            return colorConvertY2R(mvS);
    }
}

int exitColorConvert(MovieState *mvS)
{
    switch (mvS->convertColorMethod)
    {
        default:
        case CONVERT_COL_SWSCALE:
            return exitColorConvertSwscale(mvS);
        case CONVERT_COL_Y2R:
            return exitColorConvertY2R(mvS);
    }
}