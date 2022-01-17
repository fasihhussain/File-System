# File System

This was a part of the course Operating Systems (CS 232) offered by Dr. Taj Khan.

## Description
Develping a simple file system where the user can add, remove, rename, move and copy files and directories. The input file has the commands which will be executed sequentially.

The disk layout is as follows: 
- The 128 blocks are divided into 1 super block and 127 data blocks.
The superblock contains the 128 byte free block list where each byte contains a boolean value indicating whether that particular block is free or not.
- Just after the free block list, in the super block, we have the inode table containing the 16 inodes themselves. Each inode is 56 bytes in size and contains metadata about the stored files/directories as indicated by the data structure in the accompanying `filesystem.c`.
- Inodes can contain metadata about a file or a directory. The contents of a directory are the
a series of directory entries comprising of `dirent` structures

## Features
### Create a File
```
CR filename size
```

### Delete a File
```
DL filename
```

### Copy a File
```
CP srcname dstname
```

### Move a File
```
CP srcname dstname
```

### Create a Directory
```
CD dirname
```

### Remove a Directory
```
DD dirname
```

### List all Files
```
LL
```

## Sample Input
```
CD /home
CD /home/user1
CR /home/file1 1
CR /home/user1/file2 1
CP /home/file1 /home/user1/file3
DL /home/file1
CD /home/user2
MV /home/user1/file2 /home/user2/file2
LL
DD /home/user2
``` 