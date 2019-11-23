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
	uint16_t num_data_blocks;
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

uint16_t *fat_table;

struct fd_table* our_fd_table;


/* TODO: Phase 1 */

int fs_mount(const char *diskname)
{
	int i;
  void * mount_buffer=NULL;
  if(block_disk_open(diskname)==-1)
    return -1;


  block_read(0,&super);

  if(super.total_amount!=block_disk_count())
    return -1;
  if(super.num_data_blocks!=super.total_amount -2-super.num_FAT)
    return -1;
  if(super.root_index!=super.num_FAT+1)
    return -1;
  if(super.data_index!=super.root_index+1)
    return -1;


  our_fat.arr = (uint16_t *)malloc(BLOCK_SIZE*super.num_FAT*sizeof(uint16_t));
  void * temp = (void *) malloc(BLOCK_SIZE);
  for(i=1; i < super.root_index;i++)
  {
    block_read(i,temp);
    memcpy(our_fat.arr+4096*(i-1),temp,BLOCK_SIZE);
  }
  if(our_fat.arr[0]!= 0xFFFF)
    return -1;
  mount_buffer = (void *)malloc(BLOCK_SIZE);
  our_root = (struct rootdirectory *)malloc(sizeof(struct rootdirectory));
  block_read(super.root_index,mount_buffer);

  for (i=0;i<FS_FILE_MAX_COUNT;i++)
  {
    struct entries * entry = (struct entries*)malloc(sizeof(struct entries));
    memcpy(entry,mount_buffer+(i*32),32);
    our_root->root[i] = *entry;
  }

  return 0;
}

int fs_umount(void)
{
	int i;
  void * umount_buffer=malloc(sizeof(BLOCK_SIZE));
  for(i=0;i<super.num_FAT;i++)
  {
    memcpy(umount_buffer,our_fat.arr+i*4096,BLOCK_SIZE);
    block_write(1+i,umount_buffer);
  }
  block_write(super.root_index,our_root);

  int close = block_disk_close();

  if (close == -1)
    return -1;
  if(our_fd_table!=NULL)
  {
    if(our_fd_table->fd_count!=0)
      return -1;
  }
  return 0;
}

int fs_info(void)
{
	if(block_disk_count()==-1)
    return -1;
  int freefat = 0, i;
  int root = 0;

  printf("File System Info:\n");
  printf("Total Block # %i\n",super.total_amount);
  printf("FAT Block # %i\n",super.num_FAT);
  printf("Root Directory Index: %i\n",super.root_index);
  printf("Data Block Index: %i\n",super.data_index);
  printf("Data Block # %i\n",super.num_data_blocks);

  for(i=0;i < super.num_data_blocks;i++)
  {
    if(our_fat.arr[i] == 0 )
    {
      freefat++;
    }
  }

  printf("Free FAT Blocks # %d/%d\n",freefat,super.num_data_blocks);
  for(i=0; i < FS_FILE_MAX_COUNT; i++)
  {
    struct entries node;
    node = our_root->root[i];
    if (strlen(node.filename) == 0)
      root++;
  }

  printf("Free Root Entries # %d/%d\n",root ,FS_FILE_MAX_COUNT);

  return 0;
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
  int valid;

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

	//check if file named @filename already exists
	for(i = 0; i < FS_FILE_MAX_COUNT; i++){
		struct entries node;
		node = our_root->root[i];
		if(strcmp(node.filename, filename) == 0){
			return -1;
		}
	}

	//check if string @filename is too long
	if(strnlen(filename, FS_FILENAME_LEN) >= FS_FILENAME_LEN){
		return -1;
	}

	// check if root directory already contains %FS_FILE_MAX_COUNT files
	for(i = 0; i < FS_FILE_MAX_COUNT; i++){
		struct entries node;
		node = our_root->root[i];
		if(node.filename[0] != '\0'){
			int count_num_file = 0;
			count_num_file++;
			if(count_num_file >= FS_FILE_MAX_COUNT){
				return -1;
			}
		}
	}


  //create new file
  for(i = 0; i < FS_FILE_MAX_COUNT; i++){
		struct entries node;
		node = our_root->root[i];
    if(node.filename[0] == '\0'){
      strcpy(node.filename, filename);
      node.first_index = 0xFFFF;
			our_root->root[i] = node;
      block_write(super.root_index, our_root->root);
			break;
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
	int valid = 0;
	uint16_t temp;



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
  //checks if there is no file named @filename to delete
  for(i = 0; i < FS_FILE_MAX_COUNT; i++){
		struct entries entry;
		entry = our_root->root[i];
    if(strcmp(entry.filename, filename) == 0){
      check_if_file_exists = 1;
    }
  }
  if(check_if_file_exists != 1){
    return -1;
  }

  //TODO: check if file @filename is currently open
	// for(i = 0; i < FS_OPEN_MAX_COUNT; i++){
	// 	if()
	// }


	//todo delete file
	for(i = 0; i < FS_FILE_MAX_COUNT; i++){
		struct entries entry;
		entry = our_root->root[i];
		if(entry.filename[0] != '\0'){
			if(strcmp(entry.filename, filename) == 0){
				 //delete file and claer FAT
				entry.filename[0] = '\0';
				uint16_t our_FAT_index = entry.first_index;
				while(our_FAT_index != 0xFFFF){
					temp = fat_table[our_FAT_index];
					fat_table[our_FAT_index] = 0;
					our_FAT_index = temp;
				}
				break;
			}
		}
	}
	return 0;
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
	if(block_disk_count() == -1){
		return -1;
	}

	printf("FS Ls:\n");

	for(int i = 0; i < FS_FILE_MAX_COUNT; i++) {
		struct entries node;
		node = our_root->root[i];
		if (node.filename[0] != '\0') {

			printf("file: %s, ", (char*)node.filename);
			printf("size: %u, ", node.file_size);
			printf("data_blk: %u\n", node.first_index);
		}
	}
	return 0;
}

int fs_open(const char *filename)
{
	int found_empty=0;
  if (filename == NULL)
    return -1;
    
  if (our_fd_table == NULL)
  {
    our_fd_table = (struct fd *)malloc(sizeof(struct fd));
    int i;
    for (i=0; i<FS_OPEN_MAX_COUNT; i++)
    {
      struct fd_node * n = (struct fd_node*)malloc(sizeof(struct fd_node));
      our_fd_table->descriptors[i] = *n;
    }
    our_fd_table->fd_count = 0;
  }
  
  if (FS_OPEN_MAX_COUNT == our_fd->fd_count)
    return -1;
  
  int descriptor=0, i;
  for (i = 0; i < FS_OPEN_MAX_COUNT; i++)
  {
    struct fd_node node;
    node = our_fd->descriptors[i];
    if (node.filename == NULL)
    {
      node.filename = filename;
      descriptor = i;
      node.fd = fd;
      node.offset=0;
      our_fd->descriptors[i] = node;
      found_empty = 1;
    }
      
  }
  if(found_empty == 0)
    return -1;
  our_fd_table->fd_count +=1; 
  return descriptor;  
}

int fs_close(int fd)
{
	int index; 
  int found_file = 0;
  if(fd >= 32 || fd < 0)//check if out of bounds
    return -1;
  
  for (index = 0; index < FS_OPEN_MAX_COUNT; index++)
  {
    struct fd_node node;
    node = our_fd->descriptors[index];
    if (node.fd == fd)
    {
      our_fd->descriptors[index].filename = NULL;
      found_file = 1;
    } 
  }
  if (found_file == 0) //not found
    return -1;
    
  our_fd->fd_count -= 1;
    
  return 0;
}

int fs_stat(int fd)
{
	int found_file=0;
  int index;
  struct fd_node node;
  if(fd >= 32 || fd < 0)
    return -1;
  for(index=0;index<FS_OPEN_MAX_COUNT;index++)
  {
    node = our_fd->descriptors[index];
    if(node.fd==fd)
    {
      found_file = 1;
      break;
    }
  }
  
  if (found_file == 0)
    return -1;
  
  for(index = 0;index < FS_FILE_MAX_COUNT;index++)
  {
    struct entries entry;
    entry = our_root->root[i];
    if(strcmp(entry.filename,node.filename) == 0)
    {
      return entry.file_size;
    }
  }
  return 0;
}

int fs_lseek(int fd, size_t offset)
{
	int found_file = 0;
  int index;
  if(fd >= 32 || fd < 0)
    return -1;
    
  struct fd_node node;
  
  for(index=0;index<FS_OPEN_MAX_COUNT;index++)
  {
    node = our_fd->descriptors[i];
    if(node.fd==fd)
    {
      found_file = 1;
      break;
    }
  }
  
  if (found_file == 0)
    return -1;
    
  for (index=0; index < FS_FILE_MAX_COUNT; index++)
  {
      struct entries entry;
      entry = our_root->root[i];
      if (strcmp(node.filename, entry.filename) == 0)
      {
        if (offset > entry.file_size)
          return -1;
      } 
  }
  
  node.offset = offset;
  
  return 0;
}






/**
 * fs_write - Write to a file
 * @fd: File descriptor
 * @buf: Data buffer to write in the file
 * @count: Number of bytes of data to be written
 *
 * Attempt to write @count bytes of data from buffer pointer by @buf into the
 * file referenced by file descriptor @fd. It is assumed that @buf holds at
 * least @count bytes.
 *
 * When the function attempts to write past the end of the file, the file is
 * automatically extended to hold the additional bytes. If the underlying disk
 * runs out of space while performing a write operation, fs_write() should write
 * as many bytes as possible. The number of written bytes can therefore be
 * smaller than @count (it can even be 0 if there is no more space on disk).
 *
 * Return: -1 if file descriptor @fd is invalid (out of bounds or not currently
 * open). Otherwise return the number of bytes actually written.
 */
int fs_write(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */

	//check if fd is out of bounds
	if(fd < 0){
		return -1
	}
	if(fd >= FS_OPEN_MAX_COUNT){
		return -1;
	}

	//TODO check if file is not currently open


}




/**
 * fs_read - Read from a file
 * @fd: File descriptor
 * @buf: Data buffer to be filled with data
 * @count: Number of bytes of data to be read
 *
 * Attempt to read @count bytes of data from the file referenced by file
 * descriptor @fd into buffer pointer by @buf. It is assumed that @buf is large
 * enough to hold at least @count bytes.
 *
 * The number of bytes read can be smaller than @count if there are less than
 * @count bytes until the end of the file (it can even be 0 if the file offset
 * is at the end of the file). The file offset of the file descriptor is
 * implicitly incremented by the number of bytes that were actually read.
 *
 * Return: -1 if file descriptor @fd is invalid (out of bounds or not currently
 * open). Otherwise return the number of bytes actually read.
 */
int fs_read(int fd, void *buf, size_t count)
{
	/* TODO: Phase 4 */


	//check if fd is out of bounds
	if(fd < 0){
		return -1
	}
	if(fd >= FS_OPEN_MAX_COUNT){
		return -1;
	}

	//TODO check if file is not currently open






}
