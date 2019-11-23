# ECS 150 Project 4
This is our report of the ECS 150 Project 4 about implementing a simple file system.

## Data structures
We made a superblock struct:

```
struct superblock{
	uint64_t signature;
	uint16_t total_amount;
	uint16_t root_index;
	uint16_t data_index;
	uint16_t num_data_blocks;
	uint8_t num_FAT;
	uint8_t padding[4079];
}__attribute__((packed));
```
which includes the signature (which must be equal to "ECS150FS"), the total amount of blocks of virtual disk, the root directory block index, the data block start index, the amount of data blocks, the number of blocks for FAT, and the unused/padding.

We also made a root directory struct:

```
struct entries{
	char filename[16];
	uint32_t file_size;
	uint16_t first_index;
	uint8_t padding[10];
}__attribute__((packed));

struct rootdirectory{
	 struct entries root[128];
}__attribute__((packed));
```
which includes the filename, the size of the file, the index of the first data block, and the unused/padding.


Finally, we have the file descriptor structs:

```
struct fd_node{
	int fd;
	size_t offset;
	const char* filename;
};

struct fd_table{
	int fd_count;
	struct fd_node descriptors[32];
};
```

## Phase 1


## Phase 2
In fs_create, we first check for the validity of the filename. We have a for loop that goes until we've reached the length of the filename and checks if we've reached a null character. If there is no null character, then error. We check if the file named @filename already exists and return -1 if so. We also check if the root directory already contains %FS_FILE_MAX_COUNT files and return -1 if we do. Finally we create a new file. We make sure to set the initial size to 0 and make sure to set the first index to 0xFFFF (FAT_EOC).

In fs_delete, we of course check the errors provided in the API and return -1 if those errors are to be true. We then delete the file and make sure to clear the FAT.

In fs_ls, we just print the listing of all the files in the file system. Simple!

## Phase 3

## Phase 4
In fs_write, we check if fd is out of bounds by checking if fd > 0 or if fd >= FS_OPEN_MAX_COUNT and return -1 if so. We also check if the file is not currently open and return -1 if so.

In fs_read, we check if fd is out of bounds by checking if fd > 0 or if fd >= FS_OPEN_MAX_COUNT and return -1 if so. We also check if the file is not currently open and return -1 if so.
