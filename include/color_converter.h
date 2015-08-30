/**
 *@file color_converter.h
 *@author Lectem
 *@date 18/06/2015
 */
#pragma once

#include "movie.h"
#include <3ds.h>

int initColorConverter(MovieState* mvS);

int colorConvert(MovieState* mvS);

int exitColorConvert(MovieState* mvS);

