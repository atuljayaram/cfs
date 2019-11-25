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

struct fd_node * get_descriptor_node(int fd)
{
  int index;
  struct fd_node * node;
  for(index = 0;index < FS_OPEN_MAX_COUNT; index++)
  {
    node = &our_fd_table->descriptors[index];
    if(node->fd == fd)
      return node;
  }
  return NULL;
}

struct entries * get_entry(const char * filename)
{
  int index;
  for(index = 0;index < FS_FILE_MAX_COUNT; index++)
  {
    struct entries * entry;
    entry = &our_root->root[index];
    if(strcmp(entry->filename,filename) == 0)
      return entry;
  }
  return NULL;
}

int get_free_block(void)
{
  int index;
  for (index = 1; index < super.num_data_blocks; index++)
  {
    if (our_fat.arr[index] == 0)
    {
      return index;
    }
  }

  return -1; // If no free blocks
}

int allocate_free_blocks(int blocks)
{
  int index, free_count = 0;
  for (index = 1; index < super.num_data_blocks; index++)
  {
    if (our_fat.arr[index] == 0)
    {
      free_count += 1;
      if (free_count == blocks)
        return 1;
    }
  }

  return 0; // Couldn't allocate enough blocks
}

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
  int free_fat = 0, index;
  int root = 0;

  printf("FS Info:\n");
  printf("total_blk_count=%i\n",super.total_amount);
  printf("fat_blk_count=%i\n",super.num_FAT);
  printf("rdir_blk=%i\n",super.root_index);
  printf("data_blk=%i\n",super.data_index);
  printf("data_blk_count=%i\n",super.num_data_blocks);

  for(index=0;index < super.num_data_blocks;index++)
  {
    if(our_fat.arr[index] == 0 )
    {
      free_fat++;
    }
  }

  printf("fat_free_ratio=%d/%d\n",free_fat,super.num_data_blocks);
  for(index=0; index < FS_FILE_MAX_COUNT; index++)
  {
    struct entries descriptor;
    descriptor = our_root->root[index];
    if (strlen(node.filename) == 0)
      root++;
  }

  printf("rdir_free_ratio=%d/%d\n",root ,FS_FILE_MAX_COUNT);

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

  if (filename == NULL)
    return -1;

  int length = strlen(filename);

  if (length+1 > FS_FILENAME_LEN) // Invalid filename length
    return -1;

  int index, count = 0;

  for (index=0; index < FS_FILE_MAX_COUNT; index++)
  {
    struct entries descriptor;
    descriptor = our_root->root[index];
    if (strlen(descriptor.filename) != 0)
    {
      if (strcmp(node.filename,filename) == 0)
        return -1;
      count += 1;
    }
  }

  if (count == FS_FILE_MAX_COUNT) // If we have reached max capacity
      return -1;

  for (index=0; index < FS_FILE_MAX_COUNT; index++)
  {
    struct entries entry;
    entry = our_root->root[index];
    if (strlen(entry.filename) == 0)
    {
      strcpy(entry.filename, filename);
      node.first_index = 0xFFFF;
      our_root->root[index] = entry;
      block_write(super.root_index,our_root->root);
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

	if (filename == NULL)
    return -1;

  int found_file = 0;
  int index;

  if (our_fd_table != NULL)
  {
    for(index=0;index<FS_OPEN_MAX_COUNT;index++) // Check if file is open
    {
      struct fd_node node;
      node = our_fd_table->descriptors[index];
      if(strcmp(node.filename,filename)==0)
        found_file = 1;
    }

    if (found_file == 0)
      return -1;
  }

  int found_node = 0;
  for (index = 0; index < FS_FILE_MAX_COUNT; index++)
  {
    struct entries entry;
    entry = our_root->root[index];
    if(strcmp(entry.filename,filename) == 0)
    {
      found_node = 1;
      int j = 0;
      int num_blocks = entry.file_size/4096 +1;
      for (j = 0; j < num_blocks; j++)
      {
        our_fat.arr[entry.first_index+j] = 0;
        if( our_fat.arr[entry.first_index+j]==0xFFFF)
          break;
      }


      strcpy(entry.filename,"\0");
      entry.file_size=0;
      entry.first_index=0;
      our_root->root[index] = entry;

      block_write(super.root_index,our_root->root);
      break;
    }
  }
    if(found_node == 0) // Didn't exist
      return -1;

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
    our_fd_table = (struct fd_table *)malloc(sizeof(struct fd_table));
    int i;
    for (i=0; i<FS_OPEN_MAX_COUNT; i++)
    {
      struct fd_node * n = (struct fd_node*)malloc(sizeof(struct fd_node));
      our_fd_table->descriptors[i] = *n;
    }
    our_fd_table->fd_count = 0;
  }

  if (FS_OPEN_MAX_COUNT == our_fd_table->fd_count)
    return -1;

  int descriptor=0, i;
  for (i = 0; i < FS_OPEN_MAX_COUNT; i++)
  {
    struct fd_node node;
    node = our_fd_table->descriptors[i];
    if (node.filename == NULL)
    {
      node.filename = filename;
      descriptor = i;
      node.fd = descriptor;
      node.offset=0;
      our_fd_table->descriptors[i] = node;
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
    node = our_fd_table->descriptors[index];
    if (node.fd == fd)
    {
      our_fd_table->descriptors[index].filename = NULL;
      found_file = 1;
    }
  }
  if (found_file == 0) //not found
    return -1;

  our_fd_table->fd_count -= 1;

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
    node = our_fd_table->descriptors[index];
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
    entry = our_root->root[index];
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
    node = our_fd_table->descriptors[index];
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
      entry = our_root->root[index];
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
	int index;
  if(fd > 31) // If OOB
    return -1;

  struct fd_node * descriptor = get_descriptor_node(fd);

  if (descriptor == NULL)
    return -1;

  struct entries * entry = get_entry(descriptor->filename);

  if (entry == NULL)
    return -1;

  void *bouncebuffer = (void *) malloc(BLOCK_SIZE);

  entry->first_index = get_free_block();
  entry->file_size = count;
  int num_blocks = entry->file_size/4096 +1;

  int ret_count = allocate_free_blocks(num_blocks);

  if (ret_count == 0)
    return -1;

  size_t offset = count;

  for (index = 0; index < num_blocks; index++)
  {

    if(offset >= BLOCK_SIZE)
      memcpy(bouncebuffer,buf+index*4096,BLOCK_SIZE);
    else
      memcpy(bouncebuffer,buf+index*4096,offset);

    block_write(super.data_index+entry->first_index+index, bouncebuffer);

    if (offset >= BLOCK_SIZE)
      descriptor->offset += BLOCK_SIZE;
    else
      descriptor->offset += offset;

    offset -= BLOCK_SIZE;

    if (index == num_blocks - 1)
      ourFAT.arr[index+entry->first_index]=0xFFFF;
    else
     ourFAT.arr[index+entry->first_index]=index+1;
  }


  return count;
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
  if(fd>31) // Out of bounds
    return -1;
  struct fd_node * node = get_descriptor_node(fd);
  void *bouncebuffer = (void *) malloc(BLOCK_SIZE);
  if (node == NULL)
    return -1;

  size_t prev_off = node->offset; // Current offset
  node->offset = 0;

  struct entries * entry = get_entry(node->filename);

  if (entry == NULL)
    return -1;

  if (entry->file_size < count) //If file is too small
    return -1;
  int num_blocks = entry->file_size/4096 +1;


  size_t curr_offset = count;
  int index;
  for (index = 0; index < num_blocks; index++)
  {
    block_read(super.data_index+entry->first_index+index,bouncebuffer);

    if(curr_offset >= BLOCK_SIZE)
      memcpy(buf+index*4096,bouncebuffer,BLOCK_SIZE);
    else
      memcpy(buf+index*4096,bouncebuffer,curr_offset);

    if (curr_offset >= BLOCK_SIZE)
      node->offset += BLOCK_SIZE;
    else
      node->offset += curr_offset;

    curr_offset -= BLOCK_SIZE;
  }

  node->offset = prev_off;

  return count;
}
