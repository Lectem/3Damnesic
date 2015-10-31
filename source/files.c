/* 
 * File:   files.c
 * Author: amadren
 *
 * Created on 31 octobre 2015, 17:07
 */

#include <stdio.h>
#include <stdlib.h>
#include <3ds.h>
#include "draw.h"
#include "print.h"

u8* bufAdr;

char CURRENT_PATH[4096];
char FILES[40][1024]; //Max 40 files listed
Handle fsuHandle;
FS_archive sdmcArchive;
int files_max, current_file;

void unicodeToChar(char* dst, u16* src)
{
	if(!src || !dst)return;
	while(*src)*(dst++)=(*(src++))&0xFF;
	*dst=0x00;
}

//Get files from CURRENT_PATH
void initFiles(){
	draw_square(bufAdr, 0, 0, 400, 240, 0);
	Handle dirHandle;
	FS_path dirPath = FS_makePath(PATH_CHAR, CURRENT_PATH);
	FSUSER_OpenDirectory(fsuHandle, &dirHandle, sdmcArchive, dirPath);
	u32 entriesRead = 0;
	current_file = 0, files_max = 0;
	if(strcmp(CURRENT_PATH, "/")) {
		files_max = 1;
		sprintf(FILES[0], "..");
		draw_string(bufAdr, 17, 9 , 0xFFFFFF, "..");
	}
	do {
		u16 entryBuffer[512];
		char data[256];
		FSDIR_Read(dirHandle, &entriesRead, 1, entryBuffer);
		if(!entriesRead) break;
		unicodeToChar(FILES[files_max], entryBuffer);
		draw_string(bufAdr, 17, 9 + 8 * files_max, 0xFFFFFF, FILES[files_max++]);
	} while(entriesRead > 0);
	files_max--;
	FSDIR_Close(dirHandle);
}

