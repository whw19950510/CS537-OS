#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<unistd.h>
#include<getopt.h>
#include<fcntl.h>
#include "fs.h"
struct superblock* sb;

int main(int argc, char*argv[])
{
    if(argc==3) {
        if(strcmp(argv[1],"-r")!=0) {
            fprintf(stderr,"Usage: xv6_fsck -r <file_system_image>.\n");
            exit(1);
        }
        chmod(argv[2], 0666);
        int fsfd = open(argv[2], O_RDWR);
        if(fsfd < 0) {
            fprintf(stderr,"image not found.\n");
            exit(1);
        }
        struct stat st;
        fstat(fsfd, &st); 
        void *img_ptr=mmap(NULL,st.st_size,PROT_READ|PROT_WRITE,MAP_SHARED,fsfd,0);
        sb=(struct superblock*)(img_ptr+BSIZE);
        int bitblocks = sb->size/(512*8) + 1;//bitblock占用block数目
        int usedblocks = bitblocks+BBLOCK(0,sb->ninodes);
        int totalblocks = usedblocks+sb->nblocks;
        //check inode table
        struct dinode *inode_ptr=(struct dinode *)(img_ptr+2*BSIZE);
        void *bitStart = img_ptr + BBLOCK(0,sb->ninodes)*BSIZE; //start of bits
        void *dataStart = img_ptr + BBLOCK(0,sb->ninodes)*BSIZE + bitblocks*BSIZE; //start of data blocks
        void *sysEnd = dataStart+sb->nblocks*BSIZE;//file system end address
        //initialize the markup check array with all 0.
        int *inodeinuse = (int*)malloc(sizeof(int)*sb->ninodes);//mark inodes inuse situation
        int *inoderefer = (int*)malloc(sizeof(int)*sb->ninodes);
        int *targetinode = (int*)malloc(sizeof(int)*sb->ninodes);
        int *dirinode = (int*)malloc(sizeof(int)*sb->ninodes);
        int lfinum=0;
        int error=0;
        for(int i=0;i<sb->ninodes;i++) {
            inodeinuse[i]=0;
            inoderefer[i]=0;
            targetinode[i]=0;
            dirinode[i]=0;
        }
        for(int i=0;i<sb->ninodes;i++) {
            //read the root directory and find the lost_found directory
///////////////////////////////////////////////////////////////////////
            if(i==ROOTINO) {
                struct dirent* dirtemp;
                for(int j=0;j<NDIRECT;j++) {
                    if(inode_ptr->addrs[j]==0) continue;
                    dirtemp=(struct dirent*)(img_ptr+BSIZE*inode_ptr->addrs[j]);
                    for(int e=0;e<BSIZE/sizeof(struct dirent);e++) {
                        if(0==strcmp(dirtemp->name,"lost_found")) {
                            lfinum=dirtemp->inum;
                            break;
                        }
                        dirtemp++;
                    }
                    if(lfinum!=0) break;
                }
                if(lfinum==0&&inode_ptr->addrs[NDIRECT]!=0) {
                    void* indirect_ptr=img_ptr+inode_ptr->addrs[NDIRECT]*BSIZE;
                    for(int j=0;j<NINDIRECT;j++) {
                        if(((int*)indirect_ptr)[j]!=0) {
                        dirtemp = (struct dirent*)(img_ptr+((int*)indirect_ptr)[j]*BSIZE);
                            for(int e=0;e<BSIZE/sizeof(struct dirent);e++) {
                                if(0==strcmp(dirtemp->name,"lost_found")) {
                                        lfinum=dirtemp->inum;
                                        break;
                                }
                                dirtemp++;
                            }
                        if(lfinum!=0) break;
                        }
                    }
                }
            }
/////////////////////////////////////////////////////////////////////
            //find those inodes without directory reference
            if(inode_ptr->type==T_DIR) {
                dirinode[i]=1;
                struct dirent *dirtemp;
                for(int k=0;k<NDIRECT;k++) {
                    if(inode_ptr->addrs[k]==0) continue;
                    dirtemp=(struct dirent *)(img_ptr+BSIZE*inode_ptr->addrs[k]);
                    for(int e=0;e<BSIZE/sizeof(struct dirent);e++) {
                        if(dirtemp->inum!=0&&e!=0&&e!=1) {
                            inoderefer[dirtemp->inum]++;
                        }
                        dirtemp++;
                    }
                }
                if(inode_ptr->addrs[NDIRECT]!=0) {
                    //extract current indirect address
                    void* indirect_ptr=img_ptr+inode_ptr->addrs[NDIRECT]*BSIZE;
                    for(int m=0;m<NINDIRECT;m++) {
                        if(((int*)indirect_ptr)[m]!=0) {
                            dirtemp = (struct dirent*)(img_ptr+((int*)indirect_ptr)[m]*BSIZE);
                            for(int e=0;e<BSIZE/sizeof(struct dirent);e++) {
                                if(dirtemp->inum!=0) {
                                    inoderefer[dirtemp->inum]++;
                                }
                                dirtemp++;
                            }
                        }
                    }
                }
            }
            if(inode_ptr->type!=0) {
                inodeinuse[i]=1;
            } 
            inode_ptr++;
        }

        for(int i=0;i<sb->ninodes;i++) {
            if(inodeinuse[i]==1) {
                if(i!=ROOTINO&&inoderefer[i]==0) {
                    //printf("lostinode:%d\n",i);
                    targetinode[i]=1;
                    error++;
                }
            }
        }
        //printf("totalinum:%d\nlostdirinodenum:%d\ndeal with error:%d\n",sb->ninodes,lfinum,error);
        if(error!=0) {
            inode_ptr = (struct dinode *)(img_ptr+2*BSIZE);
            struct dinode* lfinode = (inode_ptr+lfinum);//address of lost found dinode
            struct dirent* dircur=(struct dirent*)(img_ptr+lfinode->addrs[0]*BSIZE);
            dircur+=2;
            for(int j=0;error>0&&j<sb->ninodes;j++) {
                if(targetinode[j]!=0) {
                    dircur->inum = j;
                    sprintf(dircur->name,"%d",error);
                    error--;
                    dircur++;
                }
            }
            
            for(int j=0;j<sb->ninodes;j++) {
                //deal with lost inode whose type are directories
                if(targetinode[j]!=0&&dirinode[j]!=0) {
                    inode_ptr = (struct dinode *)(img_ptr+2*BSIZE);
                    struct dinode* lostdir = (inode_ptr+j);
                    struct dirent* direntpar=(struct dirent*)(img_ptr+lostdir->addrs[0]*BSIZE);
                    direntpar->inum=j;//. points to itself
                    strcpy(direntpar->name,".");
                    direntpar++;
                    direntpar->inum=lfinum;//.. points to lost_found directory
                    strcpy(direntpar->name,"..");
                }
            }
            
            if(-1==msync(img_ptr,totalblocks*BSIZE,MS_SYNC)) {
                fprintf(stderr,"fail to write back to files\n");
                exit(1);
            }
            //printf("success:error:%d\n",error);
        }
        munmap(img_ptr,0);
        close(fsfd);
        free(inodeinuse);
        free(inoderefer);
        free(targetinode);
        free(dirinode);
        exit(0);   
    }
    return 0;
}