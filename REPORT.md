# ECS 150 Project 4
This is our report of the ECS 150 Project 4 about implementing a simple file system.

## Data structures
We made a superblock struct:

```
typedef struct __attribute__((__packed__)) superblock {
  uint8_t signature[SIGNATURE_SIZE];
  uint16_t total_blocks;
  uint16_t root_index;
  uint16_t data_index;
  uint16_t data_block_count;
  uint8_t fat_block_count;
  uint8_t padding[SPADDING];
} *superblock_t;
```
which includes the signature (which must be equal to "ECS150FS"), the total amount of blocks of virtual disk, the root directory block index, the data block start index, the amount of data blocks, the number of blocks for FAT, and the unused/padding.

We also made a root directory struct:

```
typedef struct __attribute__((__packed__)) root {
  uint8_t filename[FS_FILENAME_LEN];
  uint32_t size;
  uint16_t start_index;
  uint8_t padding[RPADDING];
} *root_t;
```
which includes the filename, the size of the file, the index of the first data block, and the unused/padding.


Finally, we have the cur_open struct:

```
typedef struct cur_open {
  root_t this_root;
  uint32_t offset;
} cur_open_t;
```

which contains the list of all currently open files in our system.

## Phase 1

### `fs_mount()`

We read in all of our necessary data structures and make sure that they initially contain the values we expect, as well as checking for correct formatting. We also clear our list of open files.

### `fs_umount()`

This is a 4-step process. We first check to see if there are any files open, and throw an error if there is. Then, we write all of our data to disk. We then close the disk and finally free all of our allocated data structures.

### `fs_info()`

This function simply outputs all of the information about our file system. 

## Phase 2

### `fs_create()`

We first check to see if the given filename is valid or if the file in question already exists. Then we make sure there's space in the root directory and FAT. We then allocate memory for the entry, copy the filename and start index into root directory, and sets the appropriate FAT index to FAT_EOC.

### `fs_delete()`

We first do all the standard checks to see if the file exits, and if the filename is valid. The function then deletes the file by setting all of the values in FAT corresponding to that file to 0 until we reach FAT_EOC. Afterwards, the root directory entry for the file gets set to 0 to signify it is open for reuse.

### `fs_ls()`

We simply print out all our files' sizes and data blocks.

## Phase 3

### `fs_open()`

We check to see if we have reached max capacity of open files or if the filename provided is invalid. We then find the first empty spot in list of currently open files, and proceed to add this file our list.

### `fs_close()`

This function fisrt checks to see if the providede `fd` was valid. If it is. We set all the space in memory corresponding to that file in our open files list and set it to 0. 

### `fs_stat()`

In fs_stat, all we did was return the size of the file corresponding to the specified file descriptor:

```
  return cur_open[fd].this_root->size;
```

### `fs_lseek()`

We check to see if our offset is within bounds and then adjust it accoridng to whatever value was provided:

```
cur_open[fd].offset = offset;
```

## Phase 4

### `fs_write()`

We first use the `is_fd_valid()` function we made to check if (obviously) fd is valid. If not, return -1. We then check if the file size needs to be increased in this if statement:

```
if (b_count + output_file.offset > output_file.this_root->size) {
	actual_size = set_file_block_alloc(output_file, b_count + output_file.offset);
	b_count = actual_size - output_file.offset;
}
```
where `set_file_block_alloc()` performs the actual file size calculation.

We then read the first block and change it accordingly:
```
our_block_read(block_index, block_buffer);
memcpy((block_buffer + b_offset), buffer_copy, (BLOCK_SIZE - b_offset));
our_block_write(block_index, block_buffer);

```
We then write to the blocks in the middle and finally, write to the last block using a smilar process! We then ensureto return the number of bytes actually written!


### `fs_read()`

We of course first check to see if `fd` is valid with our `is_fd_valid()` function. If not, return -1. We check if we're at the end of the file first and return 0 if so. We then allocate and fill a temporary block buffer with:

```
block_buffer = (char*) malloc(sizeof(char) * BLOCK_SIZE * block_count);
for (int index = 0; index < block_count; index++) {
	our_block_read(block_index, block_buffer + (BLOCK_SIZE*index));
	block_index = our_fat[block_index];
}
```
which we then use to copy to the main buffer and update the offset. Similar to `fs_write()`, we first check to see if we can actually read `count` bytes at all. If not, we return the bytes actually read from the file.

## Testing
We were able to pass all the test cases that the professor provides. Since the tests weren't as comprehensive though, we decided to write our own test cases! student_test1 tests a lot of things, including creating, adding, deleting files. Also, it gets the ls and cat. student_test2 tests the exact same thing, but on a larger scale. student_test3 tests the size of the filename and if the the size is illegal. student_test4 tests the removal of a file that doesn't actually exist!
