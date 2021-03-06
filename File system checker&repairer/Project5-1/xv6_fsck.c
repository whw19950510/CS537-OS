#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/mman.h>
#include<sys/stat.h>
#include<unistd.h>
#include<fcntl.h>
#include "fs.h"
struct superblock* sb;


int main(int argc,char* argv[]) {
    if(argc!=2&&argc!=3) {
        fprintf(stderr,"Usage: xv6_fsck <file_system_image>.\n");
        exit(1);
    }
    if(argc==2) {
    int fsfd = open(argv[1], O_RDONLY);
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
    int bitblocks = sb->size/(512*8) + 1;/////////bitblock占用block数目
    //usedblocks=sb->ninodes / IPB + 3 + bitblocks;//////////已经使用的block
    int usedblocks = bitblocks+BBLOCK(0,sb->ninodes);
    int totalblocks=usedblocks+sb->nblocks;
    //check inode table
    struct dinode *inode_ptr=(struct dinode *)(img_ptr+2*BSIZE);
    void *bitStart = img_ptr + BBLOCK(0,sb->ninodes)*BSIZE; //start of bits
    void *dataStart = img_ptr + BBLOCK(0,sb->ninodes)*BSIZE + bitblocks*BSIZE; //start of data blocks
    void *sysEnd = dataStart+sb->nblocks*BSIZE;//file system end address
    int cur_inodenum=0;
    //initialize the markup check array with all 0.
    int *dirreference = (int*)malloc(sizeof(int)*sb->ninodes);
    int *dirarr = (int*)malloc(sizeof(int)*sb->ninodes);
    int *visitblock = (int*)malloc(sizeof(int)*totalblocks);
    int *inodeinuse = (int*)malloc(sizeof(int)*sb->ninodes);//mark inodes inuse situation
    int *inoderefer = (int*)malloc(sizeof(int)*sb->ninodes);
    int *parentinum = (int*)malloc(sizeof(int)*sb->ninodes);
    int **childentry = (int**)malloc(sizeof(int*)*sb->ninodes);//rows of directory
    int *loopdetect=(int*)malloc(sizeof(int)*sb->ninodes);
    for(int i=0;i<sb->ninodes;i++) {
        childentry[i]=(int*)malloc(sizeof(int)*sb->ninodes);//columns of directory
    }
    for(int i=0;i<sb->ninodes;i++) {
        inodeinuse[i]=0;
        inoderefer[i]=0;
        dirreference[i]=0;
        dirarr[i]=0;
        parentinum[i]=0;
        loopdetect[i]=0;
        for(int j=0;j<sb->ninodes;j++)
            childentry[i][j]=0;
    }
    for(int i=0;i<totalblocks;i++) {
        visitblock[i]=0;
    }
    for(int i=0;i<sb->ninodes;i++) {
        if(inode_ptr->type==T_DIR) {
            dirarr[i]=1;//record all directory type inodes
        }
        inode_ptr++;
    }
    inode_ptr = (struct dinode *)(img_ptr+2*BSIZE);
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
                }
            }
            //checks indirect address is valid
            if(inode_ptr->addrs[NDIRECT]!=0) {
                visitblock[inode_ptr->addrs[NDIRECT]]++;//mark indirect block is used
                //extract current indirect address
                void* indirect_ptr=img_ptr+inode_ptr->addrs[NDIRECT]*BSIZE;
                if(indirect_ptr>sysEnd||indirect_ptr<dataStart) {
                    fprintf(stderr,"ERROR: bad indirect address in inode.\n");
                    exit(1);
                }
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
                        if(dir->inum!=0) {      //for each inuse entry
                            if(0==strcmp(dir->name,"..")) {
                                parentinum[cur_inodenum]=dir->inum;
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
                            if(dirarr[dir->inum]==1&&0!=strcmp(dir->name,"..")&&0!=strcmp(dir->name,".")) {
                                dirreference[dir->inum]++;//is a directory, count how many times is counted
                                childentry[cur_inodenum][dir->inum]=1;  //record only children directory in the current directory
                            }
                        }
                        dir++;
                    } 
                    if(findroot==0||findcur==0) {
                        fprintf(stderr,"ERROR: directory not properly formatted.\n");
                        exit(1);
                    } 
                }
                
                if(inode_ptr->addrs[NDIRECT]!=0) {
                    //extract current indirect address
                    void* indirect_ptr=img_ptr+inode_ptr->addrs[NDIRECT]*BSIZE;
                    for(int m=0;m<NINDIRECT;m++) {
                        if(((int*)indirect_ptr)[m]!=0) {
                            dir = (struct dirent*)(img_ptr+((int*)indirect_ptr)[m]*BSIZE);
                            for(int e=0;e<BSIZE/sizeof(struct dirent);e++) {
                                inoderefer[dir->inum]++;
                                if(dirarr[dir->inum]==1&&0!=strcmp(dir->name,"..")&&0!=strcmp(dir->name,".")) {
                                    dirreference[dir->inum]++;//is a directory, count how many times is counted
                                    childentry[cur_inodenum][dir->inum]=1;  //record only children directory in the current directory
                                }
                                dir++;
                            }
                        }
                    }
                }
            }
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
        if(inoderefer[i]!=0&&i!=0) {
            // exclude first unused inode
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
        if(inode_ptr->type==T_DIR) {
            //notice that dirreference shouldn't include . and .. entry.
            //but for root directory,the reference will always be 0(no reference)
            if((ROOTINO!=i&&1!=dirreference[i])||(i==ROOTINO&&0!=dirreference[i])) {
                fprintf(stderr,"ERROR: directory appears more than once in file system.\n");
                exit(1);
            }
        }
        inode_ptr++;
    }
    //check first extra rules
    for(int i=0;i<sb->ninodes;i++) {
        for(int j=0;j<sb->ninodes;j++) {
            if(childentry[i][j]==1) {
                if(parentinum[j]!=i) {
                    fprintf(stderr,"ERROR: parent directory mismatch.\n");
                    exit(1);
                }
            }
        }
    }
    //check second extra rules, no loop in directory, all refers to root directory
    //continue loop finding its parent until reach ROOTINO
    for(int i=0;i<sb->ninodes;i++) {
        if(dirarr[i]!=0&&loopdetect[i]!=1) {
            int currentde=i;
            while(parentinum[currentde]!=ROOTINO) {
                if(loopdetect[currentde]!=0) {
                    fprintf(stderr,"ERROR: inaccessible directory exists.\n");
                    exit(1);
                }
                loopdetect[currentde]++;
                currentde=parentinum[currentde];
            }
        }
    }
    /*
//////////////////////////////////////////////////////////
//scan through bitmap,acquire respective address of block&inode number
    for(int i=0;i<bitblocks;i++) {
        for(int j=0;j<BSIZE;j++) {
            for(uint m=0;m<8;m++) {
                printf("char:%x\n",(*((unsigned char*)(img_ptr+BBLOCK(0,sb->ninodes)*BSIZE+j+i*BSIZE))));
                if(((*((unsigned char*)(img_ptr+BBLOCK(0,sb->ninodes)*BSIZE+j+i*BSIZE)))&(0x1<<m))!=0) {
                    printf("char:%x\n",(*((unsigned char*)(img_ptr+BBLOCK(0,sb->ninodes)*BSIZE+j+i*BSIZE))));
                    int curblock=BBLOCK(0,sb->ninodes) + bitblocks + i + j*8 + m;
                    printf("curblock:%d visitblock:%d\n",curblock,visitblock[curblock]);
                    if(visitblock[curblock]==0) {
                        fprintf(stderr,"ERROR: bitmap marks block in use but it is not in use.\n");
                        exit(1);
                    }
                }
            }
        }
    }
    */
    for(int addr=BBLOCK(0,sb->ninodes)+1;addr<sb->nblocks;addr++) {
        if((((unsigned char*)bitStart)[addr/8]>>(addr%8))&0x1!=0) {
            if(visitblock[addr]==0) {
                fprintf(stderr,"ERROR: bitmap marks block in use but it is not in use.\n");
                exit(1);
            }
        }
    }
    munmap(img_ptr,0);
    close(fsfd);
    free(visitblock);
    free(inodeinuse);
    free(inoderefer);
    free(dirarr);
    free(dirreference);
    free(parentinum);
    for(int i=0;i<sb->ninodes;i++) {
        free(childentry[i]);//columns of directory
    }
    free(childentry);
    free(loopdetect);
    } else if(argc==3) {
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
    exit(0);
}