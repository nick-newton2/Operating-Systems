#File System

built filesystem SimpleFS with a disk emulator

## Functions in code
* fs_debug - Scan a mounted filesystem and report on how the inodes and blocks are organized
* fs_format - Creates a new filesystem on the disk, destroying any data already present. Sets aside ten percent of the blocks for inodes, clears the inode table, and writes the superblock. Returns one on success, zero otherwise. Note that formatting a filesystem does not cause it to be mounted. Also, an attempt to format an already-mounted disk should do nothing and return failure.
* fs_mount - Examine the disk for a filesystem. If one is present, read the superblock, build a free block bitmap, and prepare the filesystem for use. Return one on success, zero otherwise. Note that a successful mount is a pre-requisite for the remaining calls.
* fs_create - Create a new inode of zero length. On success, return the (positive) inumber. On failure, return zero. (Note that this implies zero cannot be a valid inumber.)
* fs_delete - Delete the inode indicated by the inumber. Release all data and indirect blocks assigned to this inode and return them to the free block map. On success, return one. On failure, return 0.
* fs_getsize - Return the logical size of the given inode, in bytes. Note that zero is a valid logical size for an inode! On failure, return -1.
* fs_read - Read data from a valid inode. Copy "length" bytes from the inode into the "data" pointer, starting at "offset" in the inode. Return the total number of bytes read. The number of bytes actually read could be smaller than the number of bytes requested, perhaps if the end of the inode is reached. If the given inumber is invalid, or any other error is encountered, return 0.
* fs_write - Write data to a valid inode. Copy "length" bytes from the pointer "data" into the inode starting at "offset" bytes. Allocate any necessary direct and indirect blocks in the process. Return the number of bytes actually written. The number of bytes actually written could be smaller than the number of bytes request, perhaps if the disk becomes full. If the given inumber is invalid, or any other error is encountered, return 0.

## Interface
Begin shell:
```sh
./simplefs mydisk 25
```

for help on options enter *help*:
```sh
simple fs> help
Commands are:
    format
    mount
    debug
    create
    delete  <inode>
    cat     <inode>
    copyin  <file> <inode>
    copyout <inode> <file>
    help
    quit
    exit
```
