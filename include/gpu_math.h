/**
 *@file gpu_math.h
 *@author Lectem
 *@date 11/07/2015
 */
#pragma once

void initOrthographicMatrix(float *m, float left, float right, float bottom, float top, float near, float far);
void SetUniformMatrix(u32 startreg, float* m);
