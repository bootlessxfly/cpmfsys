/*
 ============================================================================
 Name        : cpmfsys.c
 Author      : Christoph White
 Version     :
 Copyright   :
 Description : This was loosely based off of Dr. Qin's examples from class.
 mkDirStruct, makeFreeList, findExtentByName, and the function prototypes
 where based off Doctor Qin's example. Everything else was written by me.
 ============================================================================
 */

#include <limits.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "cpmfsys.h"

bool free_list[NUM_BLOCKS];

/*
 * Takes in a block and an index for an extent and reads
 * in a DirStruct into memory
 */
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
			// I could probably break here, but I wont
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
			// I could probably break here, but I wont
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

/*
 * Writes a directory struct back into memory. Takes in the struct,
 * extent index, and block of data. Then loads dir into the block addres in memory.
 * Once it is done, it writed to disk and repopulates the freelist array.
 */
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

/*
 * simply reads the disk block0 into memory and looks at each extent to
 * determine which blocks are used and marks those in free_list.
 */
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

/*
 * Prints out the current free_list, representing what blocks are free or used
 */
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

/*
 * Determines if a name is legal or not. This method checks if a file name
 * begins with an alphanumeric number and makes sure it contains no spaces.
 * The length of the filename is checked else where as the extention and file name
 * have different length requirements. This function does not check for a blank name,
 * that is done elsewhere.
 */
bool checkLegalName(char *name) {
	if (isalnum(name[0]) == 0 && strlen(name) > 0) {
		return false;
	}
	if (strchr(name, ' ') != NULL) {
		return false;
	}

	return true;
}

/*
 * Splits the file name into an extent and a name. Checks the length of each, checks if it is legal
 * and checks if the filename is blank. The extention is allowed to be blank
 */
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
	// File name should not be blank
	if (strlen(fname) == 0) {
		return -1;
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

/*
 * searchs each extent in block0 for a matching name. If no match is found,
 * then -1 is returned. If a bad name is found, -1 is returned.
 */
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

/*
 * Reads in each extent of the super block and printes out information
 * on each used extent
 */
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

/*
 * Does a "soft" delete of a filename if the filename is found,
 * returns 0 for sucess and -1 for failure.
 * Sets the status to unused but does not physically delete the data
 */
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

/*
 * Permenently deletes the file by overwriting the extent with zero's.
 * If no file is found, -1 is return. Else 0 is returned.
 */
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

/*
 * If a file is found, then it is renamed with
 * newName
 */
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
