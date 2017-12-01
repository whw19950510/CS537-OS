#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<unistd.h>
#include<fcntl.h>
#include "fs.h"
int fsfd;
struct superblock* sb;
int usedblocks;
int bitblocks;
int totalblocks;

int main(int argc,char* argv[]) {
    if(argc!=2) {
        fprintf(stderr,"Usage: xv6_fsck <file_system_image>.\n");
        exit(1);
    }
    fsfd = open(argv[1], O_RDONLY);
    //if file doesn't exists, exit
    if(fsfd < 0) {
        fprintf(stderr,"image not found.\n");
        exit(1);
    }
///////////////////////////////////////////////////////////////
    //calculate file size
    struct stat st;
    fstat(fsfd, &st); 
    void *img_ptr=mmap(NULL,st.st_size,PROT_READ,MAP_PRIVATE,fsfd,0);
    if(MAP_FAILED==img_ptr) {
        fprintf(stderr,"fail mapping\n");
        exit(1);
    }
/////////////////////////////////////////////////////////////////////
    sb=(struct superblock*)(img_ptr+BSIZE);
    bitblocks = sb->size/(512*8) + 1;/////////bitblock占用block数目
    //usedblocks=sb->ninodes / IPB + 3 + bitblocks;//////////已经使用的block
    usedblocks = bitblocks+BBLOCK(0,sb->ninodes);
    totalblocks=usedblocks+sb->nblocks;
    //check inode table
    struct dinode *inode_ptr=(struct dinode *)(img_ptr+2*BSIZE);
    void *bitStart = img_ptr + BBLOCK(0,sb->ninodes)*BSIZE; //start of bits
    void *dataStart = img_ptr + BBLOCK(0,sb->ninodes)*BSIZE + bitblocks*BSIZE; //start of data blocks
    void *sysEnd = dataStart+sb->nblocks*BSIZE;//file system end address
    int cur_inodenum=0;
    //initialize the markup check array with all 0.
    int *visitblock = (int*)malloc(sizeof(int)*totalblocks);
    int *datareference=(int*)malloc(sizeof(int)*totalblocks);
    int *inodeinuse = (int*)malloc(sizeof(int)*sb->ninodes);//mark inodes inuse situation
    int *inoderefer = (int*)malloc(sizeof(int)*sb->ninodes);
    for(int i=0;i<sb->ninodes;i++) {
        inodeinuse[i]=0;
        inoderefer[i]=0;
    }
    for(int i=0;i<totalblocks;i++) {
        datareference[i]=0;
        visitblock[i]=0;
    }
    for(int i=0;i<sb->ninodes;i++) {
        cur_inodenum = i;
        //check first rules
        if(inode_ptr->type!=0&&inode_ptr->type!=T_DIR&&inode_ptr->type!=T_FILE&&inode_ptr->type!=T_DEV) {
            fprintf(stderr,"ERROR: bad inode.\n");
            exit(1);
        }
        
        if(i==ROOTINO) {
            //check whether root directories exists,first inode number
            if(inode_ptr->type!=T_DIR) {
                fprintf(stderr,"ERROR: root directory does not exist.\n");
                exit(1);
            }
            struct dirent *dir;
            for(int k=0;k<NDIRECT;k++) {
                if(inode_ptr->addrs[k]==0) continue;
                dir=(struct dirent *)(img_ptr+BSIZE*inode_ptr->addrs[k]);
                for(int e=0;e<BSIZE/sizeof(struct dirent);e++) {
                        if(0==strcmp(dir->name,"..")) {
                        if(ROOTINO!=dir->inum) {
                            fprintf(stderr,"ERROR: root directory does not exist.\n");
                            exit(1);
                        }
                        break;
                    }
                    dir++;
                }
            }
        }
        
/////////////////////////////////////////////////////////////////////
////////check second rules,check whether in valid address
//Note: must check indirect blocks too, when they are in use. 
        if(inode_ptr->type!=0) {
            inodeinuse[cur_inodenum]++;//mark inode inuse
            for(int j=0;j<NDIRECT;j++) {//scan through address,only consider direct address
                if(inode_ptr->addrs[j]!=0) {//inuse address of inode
                    if(img_ptr+BSIZE*inode_ptr->addrs[j]>sysEnd||img_ptr+BSIZE*inode_ptr->addrs[j]<dataStart) {
                        fprintf(stderr,"ERROR: bad direct address in inode.\n");
                        exit(1);
                    }
                    if(++visitblock[inode_ptr->addrs[j]]>1) {
                        fprintf(stderr,"ERROR: direct address used more than once.\n");
                        exit(1);
                    }
                    //bitS is the initial address of bitMap
                    if((0x1<<(inode_ptr->addrs[j]%8))!=(*((char*)(img_ptr+BSIZE*BBLOCK(0,sb->ninodes)+inode_ptr->addrs[j]/8))&(0x1<<(inode_ptr->addrs[j]%8)))) {
                        fprintf(stderr,"ERROR: address used by inode but marked free in bitmap.\n");
                        exit(1);
                        //set #blockNumber bit in bitmap to 1
                    }
                    //datareference[inode_ptr->addrs[j]-usedblocks-1]++;
                }
            }
            //checks indirect address is valid
            if(inode_ptr->addrs[NDIRECT]!=0) {
                //extract current indirect address
                void* indirect_ptr=img_ptr+inode_ptr->addrs[NDIRECT]*BSIZE;
                if(indirect_ptr>sysEnd||indirect_ptr<dataStart) {
                    fprintf(stderr,"ERROR: bad indirect address in inode.\n");
                    exit(1);
                }
                visitblock[inode_ptr->addrs[NDIRECT]]++;//mark indirect block is used
                for(int k=0;k<NINDIRECT;k++) {
                    if(((int*)indirect_ptr)[k]!=0) {
                        if((img_ptr+((int*)indirect_ptr)[k]*BSIZE>sysEnd)||(img_ptr+((int*)indirect_ptr)[k]*BSIZE<dataStart)) {
                            fprintf(stderr,"ERROR: bad indirect address in inode.\n");
                            exit(1);
                        }
                        if(++visitblock[((int*)indirect_ptr)[k]]>1) {
                            fprintf(stderr,"ERROR: indirect address used more than once.\n");
                            exit(1);
                        }
                        //bitS is the initial address of bitMap
                        if((0x1<<(((int*)indirect_ptr)[k]%8))!=(*((char*)(img_ptr+BSIZE*BBLOCK(0,sb->ninodes)+((int*)indirect_ptr)[k]/8))&(0x1<<(((int*)indirect_ptr)[k]%8)))) {
                            fprintf(stderr,"ERROR: address used by inode but marked free in bitmap.\n");
                            exit(1);
                        }
                    }
                    //datareference[inode_ptr->addrs[k]-usedblocks-1]++;
                }
            }
            
            if(inode_ptr->type==T_DIR) {
                int findroot=0;
                int findcur=0;
                struct dirent *dir;
                for(int k=0;k<NDIRECT;k++) {
                    if(inode_ptr->addrs[k]==0) continue;
                    dir=(struct dirent *)(img_ptr+BSIZE*inode_ptr->addrs[k]);
                    for(int e=0;e<BSIZE/sizeof(struct dirent);e++) {
                        if(dir->inum!=0) {
                            if(0==strcmp(dir->name,"..")) {
                                findroot=1;
                            }
                            if(0==strcmp(dir->name,".")) {
                                if(cur_inodenum!=dir->inum) {
                                    fprintf(stderr,"ERROR: directory not properly formatted.\n");
                                    exit(1);
                                }
                                findcur=1;
                            }
                            inoderefer[dir->inum]++;
                        }
                        dir++;
                    } 
                    if(findroot==0||findcur==0) {
                        fprintf(stderr,"ERROR: directory not properly formatted.\n");
                        exit(1);
                    } 
                }
                
                if(inode_ptr->addrs[NDIRECT]!=0) {
                    //attrct current indirect address
                    void* indirect_ptr=img_ptr+inode_ptr->addrs[NDIRECT]*BSIZE;
                    for(int m=0;m<NINDIRECT;m++) {
                        if(((int*)indirect_ptr)[m]!=0) {
                            dir = (struct dirent*)(img_ptr+((int*)indirect_ptr)[m]*BSIZE);
                            for(int e=0;e<BSIZE/sizeof(struct dirent);e++) {
                                inoderefer[dir->inum]++;
                                dir++;
                            }
                        }
                    }
                }
            }
            
            //regular file;Reference counts (number of links) for regular files match the number of times file is referred to in directories (i.e., hard links work correctly). 
            /*
            if(inode_ptr->type==T_FILE) {
                for(int m=0;m<NDIRECT;m++) {
                    if(inode_ptr->nlink!=datareference[inode_ptr->addrs[m]]) {
                        fprintf(stderr,"ERROR: bad reference count for file.\n");
                        exit(1);
                    }
                }
            }
            */
            //#9 rules
            //inodeinuse[i]++;//marked as in-use inodes
        }
        
        inode_ptr++;
    }

    inode_ptr=(struct dinode *)(img_ptr+2*BSIZE);
    for(int i=0;i<sb->ninodes;i++) {
        if(inodeinuse[i]!=0) { 
            if(inoderefer[i]<1) {
                fprintf(stderr,"ERROR: inode marked use but not found in a directory.\n");
                exit(1);
            }
        }
        if(inoderefer[i]!=0) {
            if(inodeinuse[i]==0) {
                fprintf(stderr,"ERROR: inode referred to in directory but marked free.\n");
                exit(1);
            }
        }
        if(inode_ptr->type==T_FILE) {
            if(inode_ptr->nlink!=inoderefer[i]) {
                fprintf(stderr,"ERROR: bad reference count for file.\n");
                exit(1);
            }
        }
        //if(inode_ptr->type==T_DIR) {
        //    if(1!=inoderefer[i]) {
        //        fprintf(stderr,"ERROR: directory appears more than once in file system.\n");
        //        exit(1);
        //    }
        //}
        inode_ptr++;
    }
    /*
//////////////////////////////////////////////////////////
//scan through bitmap,acquire respective address of block&inode number
    for(int i=0;i<bitblocks;i++) {
        for(int j=0;j<BSIZE;j++) {
            for(int m=0;m<8;m++) {
                if(((*((char*)(bitStart+j+i*BSIZE)))<<m)&(0x1<<m)!=0) {
                    int curaddr=i*BSIZE+j*8+m;
                    if(visitblock[curaddr]==0) {
                        fprintf(stderr,"ERROR: bitmap marks block in use but it is not in use.\n");
                        exit(1);
                    }
                }
            }
        }
    }
    */
    munmap(img_ptr,0);
    close(fsfd);
    //free(visitblock);
    free(inodeinuse);
    //free(datareference);
    free(inoderefer);
    exit(0);
}