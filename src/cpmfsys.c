//============================================================================
// Name        : Hello-World.cpp
// Author      : Christoph White
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <limits.h>
#include "cpmfsys.h"

DirStructType *mkDirStruct(int index,uint8_t *e) {
	DirStructType *dir = malloc(sizeof(*dir));
	uint8_t *dir_addr = e + index * EXTENT_SIZE;
	dir->status = dir_addr[0];
	for (int i = 1; i < 9; i++) {
		char c = dir_addr[i];
		if (c != ' ') {
			dir->name[i - 1] = c;
		}
		else {
			dir->name[i - 1] = '\0';
			// I could probably break here, but for now wont.
		}
	}
	dir->name[8] = '\0';
	for (int i = 9; i < 12; i++) {
		char c = dir_addr[i];
		if (c != ' ') {
			dir->extension[i - 9] = c;
		}
		else {
			dir->extension[i - 9] = '\0';
			// I could probably break here, but for now wont.
		}
	}
	dir->extension[3] = '\0';
	dir->XL = dir_addr[12];
	dir->BC = dir_addr[13];
	dir->XH = dir_addr[14];
	dir->RC = dir_addr[15];
	for (int i = 16; i < 32; i++) {
		dir->blocks[i - 16] = dir_addr[i];
	}
	return dir;
}

void writeDirStruct(DirStructType *d, uint8_t index, uint8_t *e) {
	uint8_t *dir_addr = e + index * EXTENT_SIZE;
	dir_addr[0] = d->status;
	for (int i = 1; i < 9; i++) {
		uint8_t c = d->name[i - 1];
		if (c != '\0') {
			dir_addr[i] = c;
		}
		else {
			dir_addr[i] = ' ';
		}
	}

	for (int i = 9; i < 12; i++) {
		uint8_t c = d->extension[i - 9];
		if (c != '\0' ) {
			dir_addr[i] = c;
		}
		else {
			dir_addr[i] = ' ';
		}
	}
	dir_addr[12] = d->XL;
	dir_addr[13] = d->BC;
	dir_addr[14] = d->XH;
	dir_addr[15] = d->RC;
	for (int i = 16; i < 32; i++) {
		dir_addr[i] = d->blocks[i - 16];
	}
}

void makeFreeList() {

}

void printFreeList() {

}

bool checkLegalName(char *name) {

	return false;
}

int findExtentWithName(char *name, uint8_t *block0) {

	return 0;
}

void cpmDir() {
	uint8_t *block0 = malloc(100000);
	int block_number;
	uint8_t RC;
	uint8_t BC;
	int file_length;
	blockRead(block0, 0);
	printBlock(0);
	puts("DIRECTORY LISTING");
	for (int i = 0; i < EXTENT_SIZE; i++) {
		DirStructType *dir = mkDirStruct(i, block0);
		if (dir->status != 0xe5) {
			block_number = 0;
			for (int bl_index = 0; bl_index < 16; bl_index++) {
				if (dir->blocks[bl_index] != 0) {
					block_number++;
				}
			}
			file_length = (block_number - 1) * 1024 + dir->RC * 128 + dir->BC;
			printf("%s.%s %d\n", dir->name, dir->extension, file_length);
		}

	}

}

int cpmDelete(char *name) {

	return 0;
}

int cpmRename(char *oldName, char * newName) {

	return 0;
}
