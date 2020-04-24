//============================================================================
// Name        : Hello-World.cpp
// Author      : Christoph White
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <limits.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "cpmfsys.h"

bool free_list[NUM_BLOCKS];

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
	blockWrite(e, 0);
	makeFreeList();
}

void makeFreeList() {
	uint8_t *block0 = malloc(BLOCK_SIZE);
	/*
	 * Since this is global, I reintilialize the array as all used on each call of this function.
	 */
	for (int i = 0; i < BLOCK_SIZE; i++) {
		free_list[i] = true;
	}
	// Set Directory Block as used
	free_list[0] = false;

	blockRead(block0, 0);
	for (int i = 0; i < EXTENT_SIZE; i++) {
		DirStructType *dir = mkDirStruct(i, block0);
		if (dir->status != 0xe5) {
			for (int j = 0; j < 16; j++) {
				if (dir->blocks[j] != 0) {
					free_list[dir->blocks[j]] = false;
				}
			}
		}

		free(dir);
	}

	free(block0);




}

void printFreeList() {
	int block_count = 0;
	puts("FREE BLOCK LIST: (* means in-use");
	for (int i = 0; i < NUM_BLOCKS / 16; i++) {
		printf("%x0: ", i);
		for (int j = 0; j < NUM_BLOCKS / 16; j++) {
			if (free_list[block_count] == true) {
				printf(". ");
			}
			else {
				printf("* ");
			}
			block_count++;
		}
		printf("\n");
	}
}

bool checkLegalName(char *name) {
	if (isalnum(name[0]) == 0 && strlen(name) > 0) {
		return false;
	}
	if (strchr(name, ' ') != NULL) {
		return false;
	}

	return true;
}

int splitCheckName(char *name, char *fname, char *extName) {
	bool checkNameLength = true;
	int extention_index = 0;
	int name_pad = 0;
	for (int i = 0; i < strlen(name); i++) {
		if (name[i] != '.' && checkNameLength == true) {
			fname[i] = name[i];
		}
		else if (checkNameLength == true) {
			checkNameLength = false;
			name_pad = i;
		}
		else if (checkNameLength == false) {
			extName[extention_index] = name[i];
			extention_index++;
		}
	}
	/*
	 * Pad the name and extention
	 */
	if (name_pad == 0) {
		name_pad = strlen(name);
	}
	for (int i = name_pad; i < 9; i++) {
		fname[i] = '\0';
	}
	for (int i = extention_index; i < 3; i++) {
		extName[i] = '\0';
	}
	if (strlen(fname) > 8) {
		return -1;
	}
	if (strlen(extName) > 3) {
		return -1;
	}
	if (checkLegalName(fname) == false
			|| checkLegalName(extName) == false) {
		return -1;
	}
	return 0;
}

int findExtentWithName(char *name, uint8_t *block0) {
	char *fname = malloc(strlen(name));
	char *extName = malloc(strlen(name));
	if (splitCheckName(name, fname, extName) < 0) {
		free(fname);
		free(extName);
		return -1;
	}
	for (int i = 0; i < EXTENT_SIZE; i++) {
		DirStructType *dir = mkDirStruct(i, block0);
		if ((strcmp(dir->name, fname) == 0)
				&& (strcmp(dir->extension, extName) == 0)
				&& dir->status != 0xe5) {
			free(dir);
			free(fname);
			free(extName);
			return i;
		}
		free(dir);
	}
	free(fname);
	free(extName);
	return -1;
}

void cpmDir() {
	uint8_t *block0 = malloc(BLOCK_SIZE);
	int block_number;
	int file_length;
	blockRead(block0, 0);
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

		free(dir);
	}
	free(block0);

}

int cpmDelete(char *name) {
	uint8_t *block0 = malloc(BLOCK_SIZE);
	int extent_index = -1;
	blockRead(block0, 0);
	extent_index = findExtentWithName(name, block0);
	if (extent_index < 0) {
		/*
		 * File not found or bad name
		 */
		return -1;
	}
	DirStructType *dir = mkDirStruct(extent_index, block0);
	dir->status = 0xe5;
	writeDirStruct(dir, extent_index, block0);
	free(block0);
	free(dir);
	return 0;
}

int cpmDeletePerm(char *name) {
	uint8_t *block0 = malloc(BLOCK_SIZE);
	int extent_index = -1;
	blockRead(block0, 0);
	extent_index = findExtentWithName(name, block0);
	if (extent_index < 0) {
		/*
		 * File not found or bad name
		 */
		return -1;
	}
	DirStructType *dir = mkDirStruct(extent_index, block0);
	dir->status = 0xe5;
	strcpy(dir->name, "0");
	strcpy(dir->extension, "0");
	dir->BC = '0';
	dir->RC = '0';
	dir->XH = '0';
	dir->XL = '0';
	for (int i = 0; i < 16; i++) {
		dir->blocks[i] = '0';
	}
	writeDirStruct(dir, extent_index, block0);
	free(block0);
	free(dir);
	return 0;
}

int cpmRename(char *oldName, char * newName) {
	char* fname = malloc(strlen(newName));
	char* extName = malloc(strlen(newName));
	int extent_index = -1;
	uint8_t *block0 = malloc(BLOCK_SIZE);
	if (splitCheckName(newName, fname, extName) < 0) {
		free(block0);
		free(fname);
		free(extName);
		return -1;
	}
	blockRead(block0, 0);
	extent_index = findExtentWithName(oldName, block0);
	if (extent_index < 0) {
		free(block0);
		free(fname);
		free(extName);
		return -1;
	}
	DirStructType *dir = mkDirStruct(extent_index, block0);
	strcpy(dir->name, fname);
	strcpy(dir->extension, extName);
	writeDirStruct(dir, extent_index, block0);
	free(block0);
	free(dir);
	free(fname);
	free(extName);
	return 0;
}
