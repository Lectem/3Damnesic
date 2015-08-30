/**
 *@file gpu.h
 *@author Lectem
 *@date 11/07/2015
 */
#pragma once


#include "movie.h"

void gpuInit();


void gpuExit();


void gpuRenderFrame(MovieState *mvS);

void gpuEndFrame();