
#include "fs.h"
#include "disk.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>

#define DISK_BLOCK_SIZE	   4096
#define FS_MAGIC           0xf0f03410
#define INODES_PER_BLOCK   128
#define POINTERS_PER_INODE 5
#define POINTERS_PER_BLOCK 1024

int *bitmap = NULL;
int bitmap_size;

struct fs_superblock {
	int magic;
	int nblocks;
	int ninodeblocks;
	int ninodes;
};

struct fs_inode {
	int isvalid;
	int size;
	int direct[POINTERS_PER_INODE];
	int indirect;
};

union fs_block {
	struct fs_superblock super;
	struct fs_inode inode[INODES_PER_BLOCK];
	int pointers[POINTERS_PER_BLOCK];
	char data[DISK_BLOCK_SIZE];
};

int fs_format()
{
	int size = disk_size();

	// Need at least a superblock, an inode block, and a data block
	if (size < 3)
	{
		printf("Need at least 3 blocks!\n");
		return 0;
	}
	// We are mounted
	if (bitmap != NULL)
	{
		printf("Disk already mounted!\n");
		return 0;
	}

	union fs_block block;
	block.super.magic = FS_MAGIC;
	block.super.nblocks = size;
	block.super.ninodeblocks = ceil(0.1*size);
	block.super.ninodes = INODES_PER_BLOCK*block.super.ninodeblocks;

	// Write our superblock to the first element
	disk_write(0, block.data);

	// Create empty data and write it to disk
	char zero_data[DISK_BLOCK_SIZE];
	memset(zero_data, 0, DISK_BLOCK_SIZE);

	for (int block_number = 1; block_number <= block.super.ninodeblocks; block_number++)
	{
		printf("Writing block %d\n", block_number);
		disk_write(block_number, zero_data);
	}

	return 1;
}

void fs_debug()
{
	if (bitmap == NULL)
	{
		printf("File system not mounted!\n");
		return;
	}

	union fs_block block;

	disk_read(0,block.data);

	printf("superblock:\n");
	if (block.super.magic == FS_MAGIC)
	{
		printf("	magic number is valid\n");
	}
	else
	{
		printf("	magic number is invalid\n");
		return;
	}
	printf("	%d blocks\n",block.super.nblocks);
	printf("	%d inode blocks\n",block.super.ninodeblocks);
	printf("	%d inodes\n",block.super.ninodes);

	union fs_block inode_block;
	struct fs_inode inode;
	for (int i = 1; i <= block.super.ninodeblocks; i++)
	{
		disk_read(i, inode_block.data);
		for (int inode_number = 0; inode_number < INODES_PER_BLOCK; inode_number++)
		{
			inode = inode_block.inode[inode_number];
			if (inode.isvalid)
			{
				printf("inode %d\n", (i-1)*INODES_PER_BLOCK + inode_number);
				printf("	size: %d bytes\n", inode.size);
				printf("	direct blocks:");

				// Direct pointers
				for(int direct_block = 0; direct_block * DISK_BLOCK_SIZE < inode.size && direct_block < POINTERS_PER_INODE; direct_block++)
				{
					printf(" %d", inode.direct[direct_block]);
				}

				printf("\n");

				// Go to the indirect pointer block
				if (inode.indirect != 0)
				{
					printf("	indirect block: %d\n", inode.indirect);
					union fs_block next_block;
					disk_read(inode.indirect, next_block.data);

					printf("	indirect data blocks:");
					for(int direct_block = 0; direct_block < (inode.size/DISK_BLOCK_SIZE) - POINTERS_PER_INODE; direct_block++)
					{
						printf(" %d", next_block.pointers[direct_block]);
					}		
					printf("\n");
				}
			}
		}
	}
	printf("Bitmap:\n");
	for (int i = 0; i < bitmap_size; i += 20)
	{
		for (int j = i; j < i+20 && j < bitmap_size; j++)
		{
			if(bitmap[j])
			{
				printf("%5d", j);
			}
			else
			{
				printf("     ");
			}
		}
		printf("\n");
	}
}

int fs_mount()
{
	union fs_block block;
	disk_read(0, block.data);

	if (block.super.magic == FS_MAGIC)
	{
		bitmap = calloc(block.super.nblocks, sizeof(int));
		bitmap_size = block.super.nblocks;
		bitmap[0] = 1;

		union fs_block inode_block;
		for (int i = 1; i <= block.super.ninodeblocks; i++)
		{
			disk_read(i, inode_block.data);
			bitmap[i] = 1;

			struct fs_inode inode;
			for (int inode_number = 0; inode_number < INODES_PER_BLOCK; inode_number++)
			{
				inode = inode_block.inode[inode_number];
				if (inode.isvalid)
				{
					bitmap[i] = 1;
					for(int direct_block = 0; direct_block * DISK_BLOCK_SIZE < inode.size && direct_block < POINTERS_PER_INODE; direct_block++)
					{
						bitmap[inode.direct[direct_block]] = 1;
					}
					if (inode.indirect != 0)
					{
						bitmap[inode.indirect] = 1;

						union fs_block next_block;
						disk_read(inode.indirect, next_block.data);

						for(int direct_block = 0; direct_block < (inode.size/DISK_BLOCK_SIZE) - POINTERS_PER_INODE; direct_block++)
						{
							bitmap[next_block.pointers[direct_block]] = 1;
						}
					}
				}
			}
		}
	}
	else
	{
		printf("Invalid file system\n");
		return 0;
	}

	return 1;
}

int fs_create()
{
	if (bitmap == NULL)
	{
		printf("File system not mounted!\n");
		return 0;
	}

	union fs_block superblock;
	disk_read(0, superblock.data);

	if (superblock.super.magic != FS_MAGIC)
	{
		printf("Invalid file system\n");
		return 0;
	}

	for (int i = 1; i <= superblock.super.ninodeblocks; i++)
	{
		union fs_block inode_block;
		disk_read(i, inode_block.data);

		struct fs_inode inode;
		for (int inode_number = 0; inode_number < INODES_PER_BLOCK; inode_number++)
		{
			// Can't create and return inode 0 because of how shell.c works with return and error handling
			// (shell will say the inode creation failed)
			if (i == 1 && inode_number == 0)
			{
				continue;
			}
			inode = inode_block.inode[inode_number];
			if (inode.isvalid == 0)
			{
				// Populate new inode			
				inode.size = 0;
				memset(inode.direct, 0, sizeof(inode.direct));
				inode.indirect = 0;
				inode.isvalid = 1;

				bitmap[i] = 1;
				inode_block.inode[inode_number] = inode;
				disk_write(i, inode_block.data);
				return (i-1) * INODES_PER_BLOCK + inode_number;
			}
		}
	}

	printf("No open inodes\n");
	return 0;
}

int fs_delete( int inumber )
{
	if (bitmap == NULL)
	{
		printf("File system not mounted!\n");
		return 0;
	}

	union fs_block superblock;
	disk_read(0, superblock.data);
	
	if (superblock.super.magic != FS_MAGIC)
	{
		printf("Invalid file system\n");
		return 0;
	}
	if (inumber > superblock.super.ninodeblocks * INODES_PER_BLOCK || inumber < 0)
	{
		printf("Invalid inode number\n");
		return 0;
	}

	int inode_block_index = ((int)inumber/INODES_PER_BLOCK) + 1;

	union fs_block inode_block;
	disk_read(inode_block_index, inode_block.data);

	struct fs_inode inode;
	inode = inode_block.inode[inumber % INODES_PER_BLOCK];
	if (inode.isvalid)
	{
		// Mark all of the blocks the inode points to as free in the bitmap
		for(int direct_block = 0; direct_block * DISK_BLOCK_SIZE < inode.size && direct_block < POINTERS_PER_INODE; direct_block++)
		{
			bitmap[inode.direct[direct_block]] = 0;
		}
		if (inode.indirect != 0)
		{
			bitmap[inode.indirect] = 0;

			union fs_block next_block;
			disk_read(inode.indirect, next_block.data);

			for(int direct_block = 0; direct_block < (inode.size/DISK_BLOCK_SIZE) - POINTERS_PER_INODE; direct_block++)
			{
				bitmap[next_block.pointers[direct_block]] = 0;
			}
		}

		// Invalidate inode, rewrite it to inode block
		inode.isvalid = 0;
		inode_block.inode[inumber % INODES_PER_BLOCK] = inode;
		disk_write(inode_block_index, inode_block.data);
		return 1;
	}

	printf("Inode already invalid\n");
	return 0;
}

int fs_getsize( int inumber )
{
	if (bitmap == NULL)
	{
		printf("Disk not mounted!\n");
		return -1;
	}

	union fs_block block;
	disk_read(0, block.data);

	if (block.super.magic != FS_MAGIC)
	{
		printf("Invalid file system\n");
		return -1;
	}

	int inode_block_index = ((int)inumber/INODES_PER_BLOCK) + 1;
	
	if (inode_block_index < 0 || inode_block_index > block.super.ninodeblocks)
	{
		printf("Inode number outside of limits!\n");
		return -1;
	}

	disk_read(inode_block_index, block.data);
	struct fs_inode inode = block.inode[inumber % INODES_PER_BLOCK];
	if (inode.isvalid)
	{
		return inode.size;
	}
	printf("Invalid inode number!\n");
	return -1;
}

int fs_read( int inumber, char *data, int length, int offset )
{
	if (bitmap == NULL)
	{
		printf("File system not mounted!\n");
		return 0;
	}

	// Make sure our data array is clean
	memset(data, 0, length);

	union fs_block superblock;
	disk_read(0, superblock.data);
	
	if (inumber > superblock.super.ninodeblocks * INODES_PER_BLOCK || inumber < 0)
	{
		printf("Invalid inode number\n");
		return 0;
	}
	if (superblock.super.magic != FS_MAGIC)
	{
		printf("Invalid file system\n");
		return 0;
	}

	int inode_block_index = ((int)inumber/INODES_PER_BLOCK) + 1;

	union fs_block inode_block;
	disk_read(inode_block_index, inode_block.data);
	struct fs_inode inode = inode_block.inode[inumber % INODES_PER_BLOCK];

	if (inode.isvalid)
	{
		int bytes_read = 0;
		if (offset > inode.size)
		{
			printf("Invalid offset\n");
			return 0;
		}

		int index = (int)floor(offset/DISK_BLOCK_SIZE);
		union fs_block block;

		if (inode.size < length+offset)
		{
			length = inode.size - offset;
		}

		while (index < POINTERS_PER_INODE && bytes_read < length)
		{
			int to_read = DISK_BLOCK_SIZE;
			if (to_read + bytes_read > length)
			{
				to_read = length - bytes_read;
			}

			disk_read(inode.direct[index], block.data);
			strncat(data, block.data, to_read);
			bytes_read += to_read;
			index++;
		}
		if (bytes_read < length)
		{
			union fs_block indirect_block;
			disk_read(inode.indirect, indirect_block.data);

			for(int direct_block = 0; direct_block <= (inode.size/DISK_BLOCK_SIZE) - POINTERS_PER_INODE && bytes_read < length; direct_block++)
			{
				disk_read(indirect_block.pointers[direct_block], block.data);

				int to_read = DISK_BLOCK_SIZE;
				if (to_read + bytes_read > length)
				{
					to_read = length - bytes_read;
				}

				strncat(data, block.data, to_read);
				bytes_read += to_read;
			}
		}
		return bytes_read;
	}

	printf("Invalid inode\n");
	return 0;
}

// Helper function to get the next free block, according to our bitmap
int get_free_block()
{
	for (int i = 0; i < bitmap_size; i++)
	{
		if (bitmap[i] == 0)
		{
			bitmap[i] = 1;
			return i;
		}
	}
	// Return -1 if we couldn't find a free block
	return -1;
}

int fs_write( int inumber, const char *data, int length, int offset )
{
	union fs_block superblock;
	disk_read(0, superblock.data);
	
	if (inumber > superblock.super.ninodeblocks * INODES_PER_BLOCK || inumber < 0)
	{
		printf("Invalid inode number\n");
		return 0;
	}

	if (bitmap == NULL)
	{
		printf("File system not mounted!\n");
		return 0;
	}

	int inode_block_index = ((int)inumber/INODES_PER_BLOCK) + 1;

	union fs_block inode_block;
	disk_read(inode_block_index, inode_block.data);
	struct fs_inode inode = inode_block.inode[inumber % INODES_PER_BLOCK];

	if (inode.isvalid)
	{
		int bytes_written = 0;
		int index = (int)floor(offset/DISK_BLOCK_SIZE);
		while (index < POINTERS_PER_INODE && bytes_written < length)
		{
			int block_num = get_free_block();
			if (block_num < 0)
			{
				printf("No blocks available\n");
				return bytes_written;
			}

			int to_write = DISK_BLOCK_SIZE;
			if (to_write + bytes_written > length)
			{
				to_write = length - bytes_written;
			}

			char *buf = calloc(DISK_BLOCK_SIZE, sizeof(char));
			strncpy(buf, data, to_write);
			disk_write(block_num, buf);
			free(buf);
			bytes_written += to_write;
			data += to_write;
			inode.size += to_write;
			inode.direct[index] = block_num;
			index++;
		}

		if (bytes_written < length)
		{
			union fs_block indirect_block;
			if (inode.indirect == 0)
			{
				int block_num = get_free_block();
				if (block_num < 0)
				{
					// Possibly still wrote some stuff, write new inode to disk and return size written
					printf("No blocks available\n");
					inode_block.inode[inumber % INODES_PER_BLOCK] = inode;
					disk_write(inode_block_index, inode_block.data);
					return bytes_written;
				}

				inode.indirect = block_num;
			}
			disk_read(inode.indirect, indirect_block.data);
			for(int direct_block = 0; bytes_written < length && direct_block < POINTERS_PER_BLOCK; direct_block++)
			{
				if (indirect_block.pointers[direct_block] == 0)
				{
					int block_num = get_free_block();
					if (block_num < 0)
					{
						// Possibly still wrote some stuff, write new inodes to disk and return size written
						printf("No blocks available\n");
						disk_write(inode.indirect, indirect_block.data);
						inode_block.inode[inumber % INODES_PER_BLOCK] = inode;
						disk_write(inode_block_index, inode_block.data);
						return bytes_written;
					}

					int to_write = DISK_BLOCK_SIZE;
					if (to_write + bytes_written > length)
					{
						to_write = length - bytes_written;
					}

					char *buf = calloc(DISK_BLOCK_SIZE, sizeof(char));
					strncpy(buf, data, to_write);
					disk_write(block_num, buf);
					free(buf);
					bytes_written += to_write;
					data += to_write;
					inode.size += to_write;
					indirect_block.pointers[direct_block] = block_num;
				}
			}
			disk_write(inode.indirect, indirect_block.data);
		}

		inode_block.inode[inumber % INODES_PER_BLOCK] = inode;
		disk_write(inode_block_index, inode_block.data);
		return bytes_written;
	}

	printf("Invalid inode\n");
	return 0;
}
//if u made it this far, congrats u see it works

int fs_defragment()
{
	if (bitmap == NULL)
	{
		printf("File system not mounted!\n");
		return 0;
	}

	union fs_block superblock;
	disk_read(0, superblock.data);

	if (superblock.super.magic != FS_MAGIC)
	{
		printf("Invalid file system\n");
		return 0;
	}

	// Find open spaces in the blocks
	int *open_blocks = calloc(bitmap_size, sizeof(int));
	int num_open = 0;
	int first_open = 0;
	int found_open = 0;
	for (int i = 0; i < bitmap_size; i++)
	{
		if (bitmap[i] && found_open)
		{
			for (int j = first_open; j < i; j++)
			{
				open_blocks[num_open] = j;
				num_open++;
			}
			found_open = 0;
		}
		else if(!bitmap[i] && !found_open)
		{
			first_open = i;
			found_open = 1;
		}
	}

	// Go backwards through the bitmap and find candidates for relocation
	int *candidates = calloc(num_open, sizeof(int));
	int num_candidates = 0;
	for (int i = bitmap_size-1; i >= 0 && num_candidates < num_open; i--)
	{
		// If we pass by an open space, that's one less candidate we have to look for
		// because we will essentially be removing that empty space when moving blocks
		if (i == open_blocks[num_open-1])
		{
			num_open --;
		}
		else if (bitmap[i])
		{
			candidates[num_candidates] = i;
			num_candidates++;
		}
	}

	// Move the blocks around
	union fs_block inode_block;
	for (int i = 1; i <= superblock.super.ninodeblocks; i++)
	{
		disk_read(i, inode_block.data);

		struct fs_inode inode;
		for (int inode_number = 0; inode_number < INODES_PER_BLOCK; inode_number++)
		{
			inode = inode_block.inode[inode_number];
			if (inode.isvalid)
			{
				for(int direct_block = 0; direct_block * DISK_BLOCK_SIZE < inode.size && direct_block < POINTERS_PER_INODE; direct_block++)
				{
					for (int j = 0; j < num_candidates; j++)
					{
						if (inode.direct[direct_block] == candidates[j])
						{
							union fs_block block;
							bitmap[inode.direct[direct_block]] = 0;
							disk_read(inode.direct[direct_block], block.data);
							disk_write(open_blocks[num_open-1], block.data);
							inode.direct[direct_block] = open_blocks[num_open-1];
							inode_block.inode[inode_number] = inode;
							disk_write(i, inode_block.data);
							bitmap[open_blocks[num_open-1]] = 1;
							num_open--;
							candidates[j] = -1;
						}
					}
				}
				if (inode.indirect != 0)
				{
					union fs_block next_block;
					disk_read(inode.indirect, next_block.data);

					for(int direct_block = 0; direct_block < (inode.size/DISK_BLOCK_SIZE) - POINTERS_PER_INODE; direct_block++)
					{
						for (int j = 0; j < num_candidates; j++)
						{
							if (next_block.pointers[direct_block] == candidates[j])
							{
								union fs_block block;
								bitmap[next_block.pointers[direct_block]] = 0;
								disk_read(next_block.pointers[direct_block], block.data);
								disk_write(open_blocks[num_open-1], block.data);
								next_block.pointers[direct_block] = open_blocks[num_open-1];
								disk_write(inode.indirect, next_block.data);
								bitmap[open_blocks[num_open-1]] = 1;
								num_open--;
								candidates[j] = -1;
							}
						}
					}
				}
			}
		}
	}

	free(open_blocks);
	free(candidates);

	return 1;
}