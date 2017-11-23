#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include "fs.h"
int fsfd;
struct superblock* sb;
char zeroes[512];
int freeblock;
int usedblocks;
int bitblocks;
int freeinode = 1;
int root_inode;

int main(int argc,char* argv[]) {
    if(argc!=2) {
        fprintf(stderr,"image not found.\n");
        exit(1);
    }
    fsfd = open(argv[1], O_RDONLY);
    //if file doesn't exists, exit
    if(fsfd < 0){
        fprintf(stderr,"image not found.");
        exit(1);
    }
    //calculate file size
    struct stat st;
    int rc = fstat(fsfd, &st);
    if (rc != 0) {
        fprintf(stderr,"error stat");
        return -1;
    }
    

    void *img_ptr=mmap(NULL,st.st_size,PROT_READ,MAP_PRIVATE,fsfd,0);
    //check superblocks
    //////////////because first block is unused
    sb=(struct superblock*)(img_ptr+BSIZE);
    bitblocks = sb->size/(512*8) + 1;/////////bitblock占用block数目
    usedblocks=sb->ninodes / IPB + 3 + bitblocks;//////////已经使用的block
    freeblock = usedblocks;////////////////////////?????why is this useful
    //check inode table
    struct dinode *inode_ptr=(struct dinode *)(img_ptr+2*BSIZE);
    //BBLOCK(0, sb->ninodes) exceeds inode table
    void *bitS = img_ptr + BBLOCK(0, sb->ninodes)*BSIZE; //start of bits
    //void *dataStart = img_ptr + BBLOCK(0, sb->ninodes)*BSIZE+BSIZE; //start of data blocks
    for(int i=0;i<sb->ninodes;i++) {
        if(i==ROOTINO) {
            //check whether root directories exists
            if(inode_ptr==NULL||inode_ptr->type!=T_DIR) {
                fprintf(stderr,"root directory does not exist.\n");
                exit(1);
            }
        }
        //check first rules
        if(inode_ptr->type!=0&&inode_ptr->type!=T_DIR&&inode_ptr->type!=T_FILE&&inode_ptr->type!=T_DEV) {
            fprintf(stderr,"bad inode.\n");
            exit(1);
        }
        void *datablockAddr;
        //inuse inodes,check second rules
        //Note: must check indirect blocks too, when they are in use. ??? how to check that
        if(inode_ptr->type!=0) {
            for(int j=0;j<NDIRECT+1;j++) {//////scan through address
                if(NULL==(datablockAddr=inode_ptr->addrs[j]*BSIZE+img_ptr)) {
                    fprintf(stderr,"bad address in inode.\n");
                    exit(1);
                }
                //bitS is the initial address of bitMap
                if((1<<(inode_ptr->addrs[j]%8))!=*((int*)(bitS+inode_ptr->addrs[j]/8))) {
                    fprintf(stderr,"address used by inode but marked free in bitmap.\n");
                    exit(1);
                    //set #blockNumber bit in bitmap to 1
                }
            }

        }
        //For in-use inodes, each address in use 
        //is also marked in use in the bitmap. 
        //ERROR: address used by inode but marked free in bitmap.
        inode_ptr=inode_ptr+sizeof(struct dinode*);
    }
    struct dirent dir_buf;
    struct dirent *entry;//////current inode is directory inode
	DIR* root_dir = opendir(argv[2]);
    DIR* cur_dir=root_dir;
    if(root_dir==NULL) {
        fprintf(stderr,"root directory does not exist.\n");
    }
    int cur_fd = dirfd(cur_dir);
    fchdir(cur_fd);
    while (1) {
        rc=readdir_r(cur_dir, &dir_buf, &entry);
        if(rc!=0) {
            fprintf(stderr,"error with directory");
        }
        if (entry == NULL)
			break;
        if (strcmp(entry->d_name, ".") != 0 || strcmp(entry->d_name, "..") != 0) {
            fprintf(stderr,"directory not properly formatted.\n");
            exit(1);
        }
    }

    rc=munmap(img_ptr,0);
    close(fsfd);
    return 0;
}

