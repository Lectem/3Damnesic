//Basic sprintf
u32 cast_int(char* oBuff, u32 num) {
	if (num < 9) {
		*oBuff++ = (char)(0x30 + num);
		*oBuff = '\0';
		return 1;
	} else {
		int buffer[15];
		int bSize = 1, lCount = 0;
		buffer[0] = num % 10;
		num -= num % 10;
		while (num > 0) {
			num /= 10;
			buffer[bSize++] = num % 10;
		}
		for (int i = bSize - 2; i >= 0; i--) {
			*oBuff++ = (char)(0x30 + buffer[i]);
			lCount++;
		}
		*oBuff = '\0';
		return lCount;
	}
}
u32 cast_str(char *oBuff, char *iBuff) {
	u32 lCount = 0;
	while (*iBuff != '\0') {
		*oBuff++ = *iBuff++;
		lCount++;
	}
	*oBuff = '\0';
	return lCount;
}
#include <stdarg.h>
int sprintf(char *oBuff, const char *iBuff, ...) {
	va_list args;
	va_start(args, 40);
	while (*iBuff != '\0') {
		if (*iBuff != '%') {
			*oBuff++ = *iBuff++;
		} else {
			iBuff++; //skip %
			if(*iBuff == 'd') oBuff += cast_int(oBuff, va_arg(args, u32));
			if(*iBuff == 's') oBuff += cast_str(oBuff, va_arg(args, char*));
			iBuff++;
		}
	}
	*oBuff = '\0';
	va_end(args);
	return 0;
}
//Text out-putting
#include "font.h"
void draw_char(u8 *framebuffer, u16 X, u16 Y, u32 RGB, char c) {
	if(c == ' ') return;
	int pos = c * 8;
	int i, j, b;
	for(i = 0; i < 8; i++) {
		b =	font_table[pos + i];
		for(j = 0; j < 8; j++)
			if(b & (1 << j))
				draw_pixel(framebuffer, X + (7 - j), Y + i, RGB);
	}
}
void draw_string(u8 *framebuffer, u16 X, u16 Y, u32 RGB, const char *str) {
	if(str == NULL) return;
	int sX = X;
	const char *s = str;
	while(*s) {
		if(*s == '\n') {
			X = sX;
			Y += 8;
		} else if(*s == '\t') {
			X += 8 * 4;
		} else {
			draw_char(framebuffer, X, Y, RGB, *s);
			X += 8;
		}
		++s;
	}
}
