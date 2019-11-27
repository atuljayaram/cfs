#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "disk.h"
#include "fs.h"

#define SIGNATURE "ECS150FS"
#define SIGNATURE_SIZE 8
#define SPADDING 4079
#define RPADDING 10
#define FAT_EOC 0xFFFF

typedef struct __attribute__((__packed__)) superblock {
  uint8_t signature[SIGNATURE_SIZE];
  uint16_t total_blocks;
  uint16_t root_index;
  uint16_t data_index;
  uint16_t data_block_count;
  uint8_t fat_block_count;
  uint8_t padding[SPADDING];
} *superblock_t;

typedef struct __attribute__((__packed__)) root {
  uint8_t filename[FS_FILENAME_LEN];
  uint32_t size;
  uint16_t start_index;
  uint8_t padding[RPADDING];
} *root_t;

typedef struct cur_open {
  root_t this_root;
  uint32_t offset;
} cur_open_t;

superblock_t our_super;
root_t our_root;
cur_open_t cur_open[FS_OPEN_MAX_COUNT];
uint16_t *our_fat; // Our FAT array
uint8_t open_count; // How many are open?

// Modified block_read function
int our_block_read(size_t block, void *buf)
{
  return block_read(block + our_super->data_index, buf);
}

// Modified block_write function
int our_block_write(size_t block, void *buf)
{
  return block_write(block + our_super->data_index, buf);
}

// Returns first free spot in FAT array
int find_free_fat(int start_index)
{
  for (int index = (start_index + 1); index < our_super->data_block_count; index++) {
    if (our_fat[index] == 0) {
      return index;
    }
  }

  // If full
  return FAT_EOC;
}

/* Modifies our file to allow it to contain size bytes if needed.
Just returns size if there isn't enough space. Main helper function
for block_write()
*/
int set_file_block_alloc(cur_open_t cur_open, int size)
{
  int old_count = (cur_open.this_root->size - 1) / BLOCK_SIZE;
  int new_count = (size - 1) / BLOCK_SIZE;

  if(cur_open.this_root->size == 0) {
    old_count = 0;
  }

  int fat_index = cur_open.this_root->start_index + old_count;

  our_fat[fat_index] = find_free_fat(0);

  for (int index = old_count; index < new_count; index++) {
    fat_index = our_fat[fat_index];
    our_fat[fat_index] = find_free_fat(fat_index);
    if (our_fat[fat_index] == FAT_EOC) {
      cur_open.this_root->size = (index+1) * BLOCK_SIZE;
      return ((index+1) * BLOCK_SIZE);
    }
  }

  our_fat[fat_index] = FAT_EOC;
  cur_open.this_root->size = size;
  return size;
}

int is_fd_valid(int fd)
{
  if (fd < 0 || fd >= FS_FILE_MAX_COUNT)
    return 0;

  if (cur_open[fd].this_root == NULL)
    return 0;

  return 1;
}


int is_filename_valid(const char *filename)
{
  int name_valid = 0;

  for (int index = 0; index < FS_FILENAME_LEN; index++) {
    if (filename[index] == '\0') {
      name_valid = 1;
      break;
    }
  }

  return name_valid;
}


// Returns the index of the currently open file after offset in FAT
int find_fat_index(cur_open_t cur_open)
{
  int offset = cur_open.offset / BLOCK_SIZE;
  int fat_index = cur_open.this_root->start_index;

  for (int index = 0; index < offset; index++) {
    fat_index = our_fat[fat_index];
  }

  return fat_index;
}

// Returns the index of currently open file in our open files list, if existing
int find_open_index(const char *filename)
{
  for (int index = 0; index < FS_OPEN_MAX_COUNT; index++) {
    if (cur_open[index].this_root != NULL ) {
      if (strcmp((char*)cur_open[index].this_root->filename,filename) == 0)
        return index;
    } else if (strcmp(filename,"") == 0) {
      return index;
    }
  }

  return -1;
}

// Returns the index of the wanted file in our root directory with file of filename
int find_root_index(const char *filename) {
  int root_index = -1;
  for (int index = 0; index < FS_FILE_MAX_COUNT; index++) {
    if (strcmp((char*)our_root[index].filename,filename) == 0) {
      root_index = index;
      break;
    }
  }
  return root_index;
}

int fs_mount(const char *diskname)
{
  char sig_check[SIGNATURE_SIZE + 1];

  if (block_disk_open(diskname) == -1) //Attempt to open disk
    return -1;

  our_super = (superblock_t) malloc(sizeof(uint8_t)*BLOCK_SIZE); // Read superblock
  if (block_read(0, our_super) == -1)
    return -1;

  memcpy(sig_check,our_super->signature, SIGNATURE_SIZE); // Check signature is there
  sig_check[SIGNATURE_SIZE] = '\0';
  if (strcmp(SIGNATURE, sig_check) != 0)
    return -1;

  if (block_disk_count() != our_super->total_blocks) // Check block count
    return -1;

  our_root = (root_t) malloc(sizeof(uint8_t)*BLOCK_SIZE); // Read block count
  if (block_read((our_super->data_index-1), our_root) == -1)
    return -1;

  our_fat = (uint16_t*) malloc(sizeof(uint16_t)*BLOCK_SIZE*our_super->fat_block_count); // Read FAT
  for (int i = 0; i < our_super->fat_block_count; i++) {
    block_read(1 + i, our_fat + (i * BLOCK_SIZE));
  }

  memset(cur_open,0, sizeof(cur_open_t) * FS_OPEN_MAX_COUNT); //Clear our list of open files
  open_count = 0;

  return 0;
}

int fs_umount(void)
{

  if (open_count != 0) //Any open files?
    return -1;

  // Write to disk
  if (block_write(0, our_super) == -1)
    return -1;

  if (block_write((our_super->data_index-1), our_root) == -1)
    return -1;

  for (int index = 0; index < our_super->fat_block_count; index++) {
    block_write(1 + index, our_fat + (index * BLOCK_SIZE));
  }

  if (block_disk_close() == -1)
    return -1;

  // Free all allocated memory
  free(our_super);
  free(our_root);
  free(our_fat);
  return 0;
}

int fs_info(void)
{
  int file_count = 0, fat_count = 0;

  if(block_disk_count() == -1)
    return -1;

  printf("FS Info:\n");

  printf("total_blk_count=%d\n", our_super->total_blocks);

  printf("fat_blk_count=%d\n", our_super->fat_block_count);

  printf("rdir_blk=%d\n", (our_super->data_index - 1));

  printf("data_blk=%d\n", our_super->data_index);

  printf("data_blk_count=%d\n", our_super->data_block_count);

  // Free FAT spots
  for (int index = 0; index < our_super->data_block_count; index++) {
    if (our_fat[index] != 0) {
      fat_count++;
    }
  }
  printf("fat_free_ratio=%d/%d\n", our_super->data_block_count - fat_count, our_super->data_block_count);

  // Free spots in our directory
  for (int index = 0; index < FS_FILE_MAX_COUNT; index++) {
    if (our_root[index].filename[0] != '\0') {
      file_count++;
    }
  }
  printf("rdir_free_ratio=%d/%d\n", FS_FILE_MAX_COUNT - file_count, FS_FILE_MAX_COUNT);

  return 0;
}

int fs_create(const char *filename)
{
  int empty = -1;
  int fat_index = 0;

  if (!is_filename_valid(filename))
    return -1;

  // File already there? If not, gets first available spot
  for (int index = 0; index < FS_FILE_MAX_COUNT; index++) {
    if (strcmp((char*)our_root[index].filename,filename) == 0)
      return -1;
    if (our_root[index].filename[0] == '\0' && empty == -1)
      empty = index;
  }

  // Are we exceeding max files?
  if (empty == -1)
    return -1;

  // Are we exceeding max storage?
  fat_index = find_free_fat(0);
  if (fat_index == FAT_EOC)
    return -1;

  memset(&(our_root[empty]),0,BLOCK_SIZE/FS_FILE_MAX_COUNT);
  strcpy((char*)our_root[empty].filename,filename);
  our_root[empty].start_index = fat_index;
  our_fat[fat_index] = FAT_EOC;

  return 0;
}

int fs_delete(const char *filename)
{
  int index = -1;
  uint16_t delete = 0;

  if (!is_filename_valid(filename))
    return -1;

  if (find_open_index(filename) != -1)
    return -1;

  // Does file actually exist?
  index = find_root_index(filename);
  if (index == -1)
    return -1;

  delete = our_root[index].start_index;
  while (delete != FAT_EOC) {
    uint16_t temp = our_fat[delete];
    our_fat[delete] = 0;
    delete = temp;
  }
  memset(&(our_root[index]),0,BLOCK_SIZE/FS_FILE_MAX_COUNT); // Reset all used memory

  return 0;
}

int fs_ls(void)
{
  if(block_disk_count() == -1)
    return -1;

  printf("FS Ls:\n");

  // Print file info
  for ( int index = 0; index < FS_FILE_MAX_COUNT; index++) {
    if (our_root[index].filename[0] != 0) {

      printf("file: %s, ", (char*)our_root[index].filename);

      printf("size: %u, ", our_root[index].size);

      printf("data_blk: %u\n", our_root[index].start_index);
    }
  }

  return 0;
}

int fs_open(const char *filename)
{
  int index = -1, root_index = -1;

  if (open_count == FS_OPEN_MAX_COUNT)
    return -1;

  if (!is_filename_valid(filename))
    return -1;

  //Does file already exist?
  root_index = find_root_index(filename);
  if (root_index == -1)
    return -1;

  // Find first empty spot in cur_open
  index = find_open_index("");
  cur_open[index].this_root = &(our_root[root_index]);
  cur_open[index].offset = 0;

  open_count++;

  return index;
}

int fs_close(int fd)
{
  if (!is_fd_valid(fd))
    return -1;

  memset(&cur_open[fd],0,sizeof(cur_open_t));

  open_count--;
  return 0;
}

int fs_stat(int fd)
{
  if (!is_fd_valid(fd))
    return -1;

  return cur_open[fd].this_root->size;
}

int fs_lseek(int fd, size_t offset)
{
  if (!is_fd_valid(fd))
    return -1;

  // Within bounds?
  if (offset < 0 || offset >= cur_open[fd].this_root->size)
    return -1;

  cur_open[fd].offset = offset;

  return 0;
}

int fs_write(int fd, void *buf, size_t count)
{
  char* block_buffer;
  char* buffer_copy;
  size_t block_count,block_index,b_count,b_remainder,b_offset;
  cur_open_t output_file;

  int actual_size = 0;

  if (!is_fd_valid(fd))
    return -1;

  // Set everything up
  buffer_copy = (char*) buf;
  output_file = cur_open[fd];
  b_count = count;
  block_index = find_fat_index(output_file);

  // Does filesize need to be increased?
  if (b_count + output_file.offset > output_file.this_root->size) {
    actual_size = set_file_block_alloc(output_file, b_count + output_file.offset);
    b_count = actual_size - output_file.offset;
  }

  // Setting up (Part II)
  block_count = b_count / BLOCK_SIZE + 1;
  b_offset = output_file.offset % BLOCK_SIZE;
  block_buffer = (char*) malloc(sizeof(char) * BLOCK_SIZE);

  // Read first block and modify
  our_block_read(block_index, block_buffer);
  memcpy((block_buffer + b_offset), buffer_copy, (BLOCK_SIZE - b_offset));
  our_block_write(block_index, block_buffer);

  // Write to middle blocks
  buffer_copy += (BLOCK_SIZE - b_offset);
  block_index = our_fat[block_index];
  for (int index = 1; index < (block_count-1); index++) {
    our_block_write(block_index, buffer_copy);
    buffer_copy += BLOCK_SIZE;
    block_index = our_fat[block_index];
  }

  /* Write to last block */
  if (block_index != FAT_EOC) {
    b_remainder = (b_count + output_file.offset) % BLOCK_SIZE;
    our_block_read(block_index, block_buffer);
    memcpy(block_buffer, buffer_copy, b_remainder);
    our_block_write(block_index, block_buffer);
  }

  // Update offset
  output_file.offset += b_count;

  return b_count;
}

int fs_read(int fd, void *buf, size_t count)
{
  char* block_buffer;
  size_t block_count,block_index,b_count,b_remainder,b_offset;
  cur_open_t input_file;

  if (!is_fd_valid(fd))
    return -1;

  // Set everything up
  input_file = cur_open[fd];
  b_remainder = input_file.this_root->size - input_file.offset;
  b_count = (b_remainder < count) ? b_remainder : count;
  block_count = b_count / BLOCK_SIZE + 1;
  block_index = find_fat_index(input_file);

  // Are we already at the end of our file?
  if (block_index == 0)
    return 0;

  // Allocate and fill temp buffer
  block_buffer = (char*) malloc(sizeof(char) * BLOCK_SIZE * block_count);
  for (int index = 0; index < block_count; index++) {
    our_block_read(block_index, block_buffer + (BLOCK_SIZE*index));
    block_index = our_fat[block_index];
  }

  // Copy to main buffer
  b_offset = input_file.offset % BLOCK_SIZE;
  memcpy(buf, (block_buffer + b_offset), b_count);

  // Update offset
  input_file.offset += b_count;

  return b_count;
}
