/**
 *@file video.h
 *@author Lectem
 *@date 14/06/2015
 */
#pragma once


#include <libavutil/frame.h>
#include "movie.h"

int video_open_stream(MovieState * mv);
void video_close_stream(MovieState * mv);


void display(AVFrame *pFrame);
void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame);