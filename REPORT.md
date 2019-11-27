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

## Phase 1


## Phase 2


## Phase 3
In fs_open, we check the errors provided in the API and return -1 if they are invalid. We then find the first empty spot in cur_open. Increment. Return that index!

In fs_close, check the errors and return -1 if they are invalid. We then proceed to close the file descriptor. Decrement. Done.

In fs_stat, all we did was return the size of the file corresponding to the specified file descriptor:

```
  return cur_open[fd].this_root->size;
```

Finally, in fs_lseek, we check if we're within bounds and set:

```
cur_open[fd].offset = offset;
```

## Phase 4
In fs_write, we first use the is_fd_valid function we made to check if (obviously) fd is valid. If not, return -1. We then check if the filesize needs to be increased in this if statement:

```
if (b_count + output_file.offset > output_file.this_root->size) {
	actual_size = set_file_block_alloc(output_file, b_count + output_file.offset);
	b_count = actual_size - output_file.offset;
}
```
We then read the first block and change it accordingly:
```
our_block_read(block_index, block_buffer);
memcpy((block_buffer + b_offset), buffer_copy, (BLOCK_SIZE - b_offset));
our_block_write(block_index, block_buffer);

```
We then write to the blocks in the middle and finally, write to the last block! Make sure to return the number of bytes actually written!

In fs_read, we of course check if fd is valid with our is_fd_valid function. If not, return -1. We check if we're at the end of the file first and return 0 if so. We then allocate and fill the temporary buffer with:

```
block_buffer = (char*) malloc(sizeof(char) * BLOCK_SIZE * block_count);
for (int index = 0; index < block_count; index++) {
	our_block_read(block_index, block_buffer + (BLOCK_SIZE*index));
	block_index = our_fat[block_index];
}
```
We then copy to the main buffer and update the offset. Finally, we return the bytes actually written.

## Testing
We were able to pass all the test cases that the professor provides. Since the tests weren't as comprehensive though, we decided to write our own test cases! student_test1 tests a lot of things, including creating, adding, deleting files. Also, it gets the ls and cat. student_test2 tests the exact same thing, but on a larger scale. student_test3 tests the size of the filename and if the the size is illegal. student_test4 tests the removal of a file that doesn't actually exist!
