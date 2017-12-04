Check several rules for xv6 file images.
#1
Each inode is either unallocated or one of the valid types (T_FILE, T_DIR, T_DEV). ERROR: bad inode.
#2
For in-use inodes, each address that is used by inode is valid (points to a valid datablock address within the image). If the direct block is used and is invalid, print ERROR: bad direct address in inode.; if the indirect block is in use and is invalid, print ERROR: bad indirect address in inode.
should check both indirect pointer itself and indirect block address are in valid range.
#3
Root directory exists, its inode number is 1, and the parent of the root directory is itself. ERROR: root directory does not exist.
#4
Each directory contains . and .. entries, and the . entry points to the directory itself. ERROR: directory not properly formatted.
#5
For in-use inodes, each address in use is also marked in use in the bitmap. ERROR: address used by inode but marked free in bitmap. 
Need to shift bit of 1byte(which is a char)!,when extarct corresponding bit, needs to use &.
#6
For blocks marked in-use in bitmap, actually is in-use in an inode or indirect block somewhere. ERROR: bitmap marks block in use but it is not in use.
#7
For in-use inodes, direct address in use is only used once. ERROR: direct address used more than once.
using count map for address.
#8
For in-use inodes, indirect address in use is only used once. ERROR: indirect address used more than once. Use a visit array to record reference count of each address.
#9
For all inodes marked in use, must be referred to in at least one directory. ERROR: inode marked use but not found in a directory.
#10
For each inode number that is referred to in a valid directory, it is actually marked in use. ERROR: inode referred to in directory but marked free. just in converse of 9,record inuse array and reference array,compare them whether they equal.
#11
Reference counts (number of links) for regular files match the number of times file is referred to in directories (i.e., hard links work correctly). ERROR: bad reference count for file.
count reference time for all regular files.
#12
No extra links allowed for directories (each directory only appears in one other directory). ERROR: directory appears more than once in file system.
only count directory reference except .. and .

#extra1
Each .. entry in directory refers to the proper parent inode, and parent inode points back to it. ERROR: parent directory mismatch.
record all parent inodenumber of current inode.record all children entry inodenumber of current inode. compare whether they are in consistency.

#extra2
Every directory traces back to the root directory. (i.e. no loops in the directory tree.) ERROR: inaccessible directory exists.
using while until reach rootinode or the visit map has been reached twice print error.

Structure of file image
root | superblock | inodetable | databitmap | datablock
notice:each inode with T_FILE points to some block in datablock;each inode with T_DIR points has addr/indirect addr field, indirect address points to a datablock, which is full of address,this address again points to a dirent entry in the datablock, which is part of the datablock, thus we need to scan the datablock again to find each entry's content.

for rule#10,notice that should exclude the 1st unused inode.
