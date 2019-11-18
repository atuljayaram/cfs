#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

typedef unsigned __int128 uint128_t;

struct superblock{
	uint64_t signature;
	uint16_t total_amount;
	uint16_t root_index;
	uint16_t data_index;
	uint16_t num_blocks;
	uint8_t num_FAT;
	uint8_t padding[4079];
}__attribute__((packed));

struct superblock super;

struct FAT{
	uint16_t* arr;
}__attribute__((packed));

struct FAT our_fat;

struct entries{
	char filename[16];
	uint32_t file_size;
	uint16_t first_index;
	uint8_t padding[10];
}__attribute__((packed));

struct rootdirectory{
	 struct entries root[128];
}__attribute__((packed));

struct rootdirectory * our_root;

struct fd_node{
	int fd;
	size_t offset;
	const char* filename;
};

struct fd_table{
	int fd_count;
	struct fd_node descriptors[32];
};

struct fd* our_fd_table;
fd_t currently_open[FS_OPEN_MAX_COUNT];

/* TODO: Phase 1 */

int fs_mount(const char *diskname)
{
	/* TODO: Phase 1 */
}

int fs_umount(void)
{
	/* TODO: Phase 1 */
}

int fs_info(void)
{
	/* TODO: Phase 1 */
}









//done?
/**
 * fs_create - Create a new file
 * @filename: File name
 *
 * Create a new and empty file named @filename in the root directory of the
 * mounted file system. String @filename must be NULL-terminated and its total
 * length cannot exceed %FS_FILENAME_LEN characters (including the NULL
 * character).
 *
 * Return: -1 if @filename is invalid, if a file named @filename already exists,
 * or if string @filename is too long, or if the root directory already contains
 * %FS_FILE_MAX_COUNT files. 0 otherwise.
 */
int fs_create(const char *filename)
{

  int i = 0;
  int count = 0;
  int valid = 1;


  /* TODO: Phase 2 */


  //checks for validity of filename
  for(i = 0; i < FS_FILENAME_LEN; i++){
    if(filename[i] == '\0'){
      valid = 1;
      break;
    }
  }
  if(valid != 1){
    return -1;
  }

  //checks if the file named @filename already exists
  for(i = 0; i < FS_FILE_MAX_COUNT; i++){
    if(strlen(root[i].filename, filename) != 0){
      if(strcmp(root[i].filename, filename) == 0){
        return -1;
      }
      else{
        count = count + 1;
      }
    }

    //string of filename is too long
    if((strlen(filename)) >= FS_FILENAME_LEN){
      return -1;
    }

    //check if root directory already contains FS_FILE_MAX_COUNT file_size
    if(count == FS_FILE_MAX_COUNT){
      return -1;
    }
  }

  //create new file
  for(i = 0; i < FS_FILE_MAX_COUNT; i++){
    if(root[i].filename[0] == '\0'){
      strcpy(root[i].filename, filename);
      root[i].first_index = 0xFFFF;
      root[i].file_size = 0;
      block_write(super.root_index, &root);
    }
  }
  return 0;
}



/**
 * fs_delete - Delete a file
 * @filename: File name
 *
 * Delete the file named @filename from the root directory of the mounted file
 * system.
 *
 * Return: -1 if @filename is invalid, if there is no file named @filename to
 * delete, or if file @filename is currently open. 0 otherwise.
 */
int fs_delete(const char *filename)
{
	/* TODO: Phase 2 */
  int i = 0;
  int check_if_file_exists = 1;

  //checks for validity of filename
  for(i = 0; i < FS_FILENAME_LEN; i++){
    if(filename[i] == '\0'){
      int valid = 1;
      break;
    }
  }
  if(valid != 1){
    return -1;
  }
  //checks if there is no file named @filename to delete
  for(i = 0; i < FS_FILE_MAX_COUNT; i++){
    if(strcmp(root[i].filename, filename) == 0){
      check_if_file_exists = 1;
    }
  }
  if(check_if_file_exists != 1){
    return -1
  }

  //check if file @filename is currently open
  for(i = 0; i < FS_OPEN_MAX_COUNT; i++){
    if(strcmp(currently_open[i].filename, filename) == 0){
      if(currently_open[i].fd != -1){
        return -1;
      }
    }
  }
}




/**
 * fs_ls - List files on file system
 *
 * List information about the files located in the root directory.
 *
 * Return: -1 if no underlying virtual disk was opened. 0 otherwise.
 */
int fs_ls(void)
{
	/* TODO: Phase 2 */
}

















int fs_open(const char *filename)
{
	/* TODO: Phase 3 */
}

int fs_close(int fd)
{
	/* TODO: Phase 3 */
}

int fs_stat(int fd)
{
	/* TODO: Phase 3 */
}

int fs_lseek(int fd, size_t offset)
{
	/* TODO: Phase 3 */
}

int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}

int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */
}
