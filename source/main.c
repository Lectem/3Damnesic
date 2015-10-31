#include <3ds.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include "movie.h"
#include "gpu.h"
#include "video.h"
#include "color_converter.h"
#include "files.h"


void waitForStart()
{
    while (aptMainLoop())
    {
        gspWaitForVBlank();
        hidScanInput();
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START)break;
    }
}


void initServices()
{
    hidInit(NULL);
    sdmcInit();
    gfxInitDefault();
    consoleInit(GFX_BOTTOM, NULL);

//    gfxInit(GSP_RGBA8_OES,GSP_BGR8_OES,false);
//    gfxInit(GSP_BGR8_OES,GSP_BGR8_OES,false);

    printf("Initializing the GPU...\n");
    gpuInit();
    printf("Done.\n");
}

void exitServices()
{
    gpuExit();
    gfxExit();
    sdmcExit();
    hidExit();
}

void waitForStartAndExit()
{
    printf("Press start to exit\n");
    waitForStart();
    exitServices();
}


int main(int argc, char *argv[])
{
//    char filename[]="/test400x240-mpeg4-witch.mp4";
//    char filename[]="/test400x240-witch.mp4";
//    char filename[]="/test800x400-witch-900kbps.mp4";
//    char filename[]="/test800x400-witch-1pass.mp4";
//    char filename[]="/test800x400-witch.mp4";
//    char filename[]="/test800x480-witch-mpeg4.mp4";
//    char filename[]="/test320x176-karanokyoukai.mp4";
    char filename[] = "/test.mp4";

    MovieState mvS;

    initServices();

    // Register all formats and codecs
    av_register_all();
    av_log_set_level(AV_LOG_INFO);
    printf("Listing the files...\n");
    initFiles();
    printf("Done.\n");
    printf("Press start to open the file\n");
    waitForStart();
    int ret = setup(&mvS, filename);
    if (ret)
    {
        waitForStartAndExit();
        return -1;
    }

    printf("Press start to decompress\n");
    waitForStart();
    // Read frames and save first five frames to disk
    int i = 0;
    int frameFinished;

    u64 timeBeginning, timeEnd;
    u64 timeBefore, timeAfter;
    u64 timeDecodeTotal = 0, timeScaleTotal = 0, timeDisplayTotal = 0;

    timeBefore = osGetTime();
    timeBeginning = timeBefore;
    bool stop = false;

    while (av_read_frame(mvS.pFormatCtx, &mvS.packet) >= 0 && !stop)
    {
        // Is this a packet from the video stream?
        if (mvS.packet.stream_index == mvS.videoStream)
        {

            /*********************
             * Decode video frame
             *********************/

            int err = avcodec_decode_video2(mvS.pCodecCtx, mvS.pFrame, &frameFinished, &mvS.packet);
            if (err <= 0)printf("decode error\n");
            // Did we get a video frame?
            if (frameFinished)
            {
                err = av_frame_get_decode_error_flags(mvS.pFrame);
                if (err)
                {
                    char buf[100];
                    av_strerror(err, buf, 100);
                }
                timeAfter = osGetTime();
                timeDecodeTotal += timeAfter - timeBefore;

                /*******************************
                 * Conversion of decoded frame
                 *******************************/
                timeBefore = osGetTime();
                colorConvert(&mvS);
                timeAfter = osGetTime();

                /***********************
                 * Display of the frame
                 ***********************/
                timeScaleTotal += timeAfter - timeBefore;
                timeBefore = osGetTime();

                if (mvS.renderGpu)
                {
                    gpuRenderFrame(&mvS);
                    gpuEndFrame();
                }
                else display(mvS.outFrame);

                timeAfter = osGetTime();
                timeDisplayTotal += timeAfter - timeBefore;

                ++i;//New frame

                hidScanInput();
                u32 kDown = hidKeysDown();
                if (kDown & KEY_START)
                    stop = true; // break in order to return to hbmenu
                if (i % 50 == 0)printf("frame %d\n", i);
                timeBefore = osGetTime();
            }

        }

        // Free the packet that was allocated by av_read_frame
        av_free_packet(&mvS.packet);
    }
    timeEnd = timeBefore;

    tearup(&mvS);

    printf("Played %d frames in %f s (%f fps)\n",
           i, (timeEnd - timeBeginning) / 1000.0,
           i / ((timeEnd - timeBeginning) / 1000.0));
    printf("\tdecode:\t%llu\t%f perframe"
           "\n\tscale:\t%llu\t%f perframe"
           "\n\tdisplay:\t%llu\t%f perframe\n",
           timeDecodeTotal, timeDecodeTotal / (double) i,
           timeScaleTotal, timeScaleTotal / (double) i,
           timeDisplayTotal, timeDisplayTotal / (double) i);

    waitForStartAndExit();
    return 0;
}
