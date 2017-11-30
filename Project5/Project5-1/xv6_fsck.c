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
int totalblocks;
int root_inode;

int main(int argc,char* argv[]) {
    if(argc!=2) {
        fprintf(stderr,"Usage: xv6_fsck <file_system_image>.\n");
        exit(1);
    }
    fsfd = open(argv[1], O_RDONLY);
    //if file doesn't exists, exit
    if(fsfd < 0){
        fprintf(stderr,"image not found.");
        exit(1);
    }
///////////////////////////////////////////////////////////////
    //calculate file size
    struct stat st;
    int rc = fstat(fsfd, &st);
    if (rc != 0) {
        fprintf(stderr,"error stat");
        return -1;
    }
    
    void *img_ptr=mmap(NULL,st.st_size,PROT_READ,MAP_PRIVATE,fsfd,0);
/////////////////////////////////////////////////////////////////////
    //check superblocks
    //////////////because first block is unused
    sb=(struct superblock*)(img_ptr+BSIZE);
    bitblocks = sb->size/(512*8) + 1;/////////bitblock占用block数目
    usedblocks=sb->ninodes / IPB + 3 + bitblocks;//////////已经使用的block
    freeblock = usedblocks;////////////////////////?????why is this useful
    totalblocks=usedblocks+sb->nblocks;
    //check inode table
    struct dinode *inode_ptr=(struct dinode *)(img_ptr+2*BSIZE);
    //BBLOCK(0, sb->ninodes) exceeds inode table
    void *bitStart = img_ptr + 2*BSIZE + BBLOCK(0, sb->ninodes)*BSIZE; //start of bits
    void *dataStart = img_ptr + 2*BSIZE + BBLOCK(0, sb->ninodes)*BSIZE + bitblocks*BSIZE; //start of data blocks
    void *sysend=dataStart+sb->nblocks*BSIZE;//////////file system end address
    int cur_inodenum;
    for(int i=0;i<sb->ninodes;i++) {
        cur_inodenum = i+1;
        if(cur_inodenum==1) {
            //check whether root directories exists,first inode number
            if(inode_ptr==NULL||inode_ptr->type!=T_DIR) {
                fprintf(stderr,"root directory does not exist.\n");
                exit(1);
            }
            struct dirent *dir_entry=(struct dirent *)(img_ptr);
            struct dirent *dir;
            for(int k=0;k<NDIRECT;k++) {
                if(inode_ptr->addrs[k]==0) continue;
                dir=dir_entry+BSIZE*inode_ptr->addrs[k];
                if(0==strcmp(dir->name,"..")) {
                    if(1!=dir->inum) {
                        fprintf(stderr,"root directory does not exist.\n");
                        exit(1);
                    }
                    break;
                }
            }   
        }
        //check first rules
        if(inode_ptr->type!=0&&inode_ptr->type!=T_DIR&&inode_ptr->type!=T_FILE&&inode_ptr->type!=T_DEV) {
            fprintf(stderr,"bad inode.\n");
            exit(1);
        }
/////////////////////////////////////////////////////////////////////
////////check second rules,check whether in valid address
        //inuse inodes,check second rules
        //Note: must check indirect blocks too, when they are in use. ??? how to check that
        if(inode_ptr->type!=0) {
            for(int j=0;j<NDIRECT;j++) {//////scan through address,only consider direct address
                if(inode_ptr->addrs[j]!=0) {///////inuse address of inode
                    if(img_ptr+BSIZE*inode_ptr->addrs[j]>sysend||img_ptr+BSIZE*inode_ptr->addrs[j]<dataStart) {
                        fprintf(stderr,"bad direct address in inode.\n");
                    exit(1);
                    }
                }
                //inuse address
                if(inode_ptr->addrs[j]!=0){

                }
                //bitS is the initial address of bitMap
                if((1<<(inode_ptr->addrs[j]%8))!=*((int*)(bitStart+inode_ptr->addrs[j]/8))) {
                    fprintf(stderr,"address used by inode but marked free in bitmap.\n");
                    exit(1);
                    //set #blockNumber bit in bitmap to 1
                }
            }
            //checks indirect address is valid
            if(inode_ptr->addrs[NDIRECT]!=0) {
                //attrct current indirect address
                void* indirect_ptr=img_ptr+inode_ptr->addrs[NDIRECT]*BSIZE;
                for(int k=0;k<NINDIRECT;k++) {
                    if(((int*)indirect_ptr)[k]!=0) {
                        if(((int*)indirect_ptr)[k]>sysend||((int*)indirect_ptr)[k]<dataStart) {
                            fprintf(stderr,"bad indirect address in inode.\n");
                            exit(1);
                        }
                    }
                }
                /*
                int indirectaddr[NINDIRECT];
                int indirect_st=inode_ptr->addrs[NDIRECT];
                if(-1==rsect(xint(indirect_st), (char*)indirectaddr)) {
                    fprintf(stderr,"bad indirect address in inode.\n");
                    exit(1);
                }
                */
            }
            if(inode_ptr->type==T_DIR) {
                int findroot=0;
                int findcur=0;
                struct dirent *dir_entry=(struct dirent *)(img_ptr);
                struct dirent *dir;
                for(int k=0;k<NDIRECT;k++) {
                    if(inode_ptr->addrs[k]==0) continue;
                    dir=dir_entry+BSIZE*inode_ptr->addrs[k];
                    if(0==strcmp(dir->name,"..")) {
                        findroot=1;
                    }
                    if(0==strcmp(dir->name,".")) {
                        if(cur_inodenum!=dir->inum) {
                            fprintf(stderr,"directory not properly formatted.\n");
                            exit(1);
                        }
                        findcur=1;
                    }
                    if(findroot==1&&findcur==1) break;
                }
                if(findroot==0||findcur==0) {
                    fprintf(stderr,"directory not properly formatted.\n");
                    exit(1);
                }   
            }
            //regular file;Reference counts (number of links) for regular files match the number of times file is referred to in directories (i.e., hard links work correctly). 
            if(inode_ptr->type==T_FILE) {
                if(inode_ptr->nlink!=) {
                    fprintf(stderr,"bad reference count for file.\n");
                    exit(1);
                }
            }
///////////////////////////////////////////////////////////////////////
            char *p = (char*)xp;
            int fbn, off, n1;
            struct dinode din;
            char buf[512];
            int indirect[NINDIRECT];
            int x;
            int n=sizeof(struct dirent);
            off = xint(inode_ptr->size);
            while(n > 0){
                fbn = off / 512;
                if(fbn < NDIRECT){
                if(xint(inode_ptr->addrs[fbn]) == 0){
                    inode_ptr->addrs[fbn] = xint(freeblock++);
                    usedblocks++;
                }
                x = xint(inode_ptr->addrs[fbn]);
                } else {
                if(xint(inode_ptr->addrs[NDIRECT]) == 0){
                    // printf("allocate indirect block\n");
                    din.addrs[NDIRECT] = xint(freeblock++);
                    usedblocks++;
                }
                // printf("read indirect block\n");
                rsect(xint(din.addrs[NDIRECT]), (char*)indirect);
                if(indirect[fbn - NDIRECT] == 0){
                    indirect[fbn - NDIRECT] = xint(freeblock++);
                    usedblocks++;
                    wsect(xint(din.addrs[NDIRECT]), (char*)indirect);
                }
                x = xint(indirect[fbn-NDIRECT]);
                }
                n1 = min(n, (fbn + 1) * 512 - off);
                rsect(x, buf);
                bcopy(p, buf + off - (fbn * 512), n1);
                wsect(x, buf);
                n -= n1;
                off += n1;
                p += n1;
            }
            inode_ptr->size = xint(off);
            winode(inum, &din);
        }
        //For in-use inodes, each address in use 
        //is also marked in use in the bitmap. 
        //ERROR: address used by inode but marked free in bitmap.
        inode_ptr=inode_ptr+sizeof(struct dinode*);
    }
    rc=munmap(img_ptr,0);
    close(fsfd);
    return 0;
}
        
        //For in-use inodes, each address in use 
        //is also marked in use in the bitmap. 
        //ERROR: address used by inode but marked free in bitmap.

    //////////after scan the inode table;;;read the directory and some statictics
    
uint
xint(uint x)
{
  uint y;
  char *a = (char*)&y;
  a[0] = x;
  a[1] = x >> 8;
  a[2] = x >> 16;
  a[3] = x >> 24;
  return y;
}

int 
rsect(uint sec, void *buf)
{
  if(lseek(fsfd, sec * 512L, 0) != sec * 512L){
    return -1;
  }
  if(read(fsfd, buf, 512) != 512){
    return -1;
  }
  return 0;
}