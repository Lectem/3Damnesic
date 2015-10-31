void draw_pixel(u8 *framebuffer, u16 X, u16 Y, u32 RGB) {
	u32 pos = (240 - Y) * 3 + X * 3 * 240;
	framebuffer[pos + 0] = RGB & 0xFF;
	framebuffer[pos + 1] = (RGB >> 8) & 0xFF;
	framebuffer[pos + 2] = (RGB >> 16) & 0xFF;
}
void draw_square(u8 *framebuffer, u16 X, u16 Y, u16 W, u16 H, u32 RGB) {
	int i, j;
	for(i = 0; i < W; i++)
		for(j = 0; j < H; j++)
			draw_pixel(framebuffer, X + i, Y + j, RGB);
}