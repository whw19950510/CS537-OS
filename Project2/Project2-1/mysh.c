#define _GNU_SOURCE
#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<fcntl.h>
#include<signal.h>
int main(int argc,char* argv[])
{
    char error_message[30] = "An error has occurred\n";
    int processtable[20];
    int processindex=0;
    int processnum=0;
    int m=0;
    while(m<20)processtable[m++]=1;
    int linehist=1;
    //badarg test..
    if(argc!=1)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }
    while(1)
    {
        //scan the process table so that no tombie process
        processindex=0;
        processnum=0;
        while(processindex<20)
        {
            if(0==kill(processtable[processindex],0))//process still running
            {
                processtable[processnum++]=processtable[processindex];
            }
            else
            {//zombie process, wait to clear resources
                if(processtable[processindex]!=1)
                waitpid(processtable[processindex],NULL,0);
            }
            processindex++;
        }
        //promp print
        printf("mysh (%d)> ",linehist);
        linehist++;
        fflush(stdout);
        char input[1024];
        //no characters are founded or end of file occurs
        if(fgets(input,1024,stdin)==NULL)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
        else
        {
            //detect the input line length exceeds 128 bytes
            /*
            if (input[sizeof input - 1] == '\0' && input[sizeof input - 2] != '\n')
            {
                write(STDERR_FILENO, error_message, strlen(error_message));
                continue;
            }
            */
            if(strlen(input)>128)
            {
                write(STDERR_FILENO, error_message, strlen(error_message));
                continue;
            }
            else
            {//maybe legal input
                input[strlen(input)-1]='\0';//concatate the \n, prevent execute
                char* command[128];
                char* todeal = input;
                char* reserve;
                int argu=0;                 //number of arguments in the command,\n removed
                char* token = strtok_r(todeal," \t",&reserve);
                if(token==NULL) {linehist--;continue;}  //empty command line
                else {command[argu++]=strdup(token);}
                while((token=strtok_r(NULL," \t",&reserve)))
                {
                    command[argu]=strdup(token);
                    argu++;
                }
                command[argu]=NULL;
//////////////////////////////////////////////////////////////////////////
                //parse command already
                if(strcmp(command[0],"cd")==0)   //deal with cd
                {
                    if(command[1]==NULL)
                    {
                        char* homepath=getenv("HOME");
                        if(homepath==NULL)
                        {
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            continue;
                        }
                        if(chdir(homepath)==0) {continue;}//success cd into home
                        else{write(STDERR_FILENO, error_message, strlen(error_message));continue;}
                    }
                    else
                    {
                        if(chdir(command[1])==0) continue;//success cd into some path
                        else{write(STDERR_FILENO, error_message, strlen(error_message));continue;}
                    }
                //deal with pwd
                }
                else if(strcmp(command[0],"pwd")==0)
                {
                    if(command[1]!=NULL)
                    {write(STDERR_FILENO, error_message, strlen(error_message));continue;}
                    char bufferpath[512];
                    if(getcwd(bufferpath,sizeof(bufferpath))==NULL)
                    {write(STDERR_FILENO, error_message, strlen(error_message));continue;}
                    printf("%s\n",bufferpath);
                //deal with exit
                }
                else if(strcmp(command[0],"exit")==0)
                {       
                    int l=0;
                    int returnsig;
                    while(l<20)
                    {
                        if((returnsig=kill(processtable[l],0))==0)
                            kill(processtable[l],SIGTERM);
                        l++;
                    }
                    exit(0);      
                }
                else
                {
                    //Here deal with all cases that is not built-in methods
                    //need to deal with command again, only keep command and arguments
                    int reout=-1,rein=-1,pipepos=-1,back=-1;
                    int outfilenum=0,infilenum=0;
                    int i=0;
                    while(i<argu)
                    {
                        if(strcmp(command[i],">")==0)
                        {
                            reout=i;
                            //extract outfile number
                            int j=i+1;
                            while(j<argu)
                            {//after retrieve the outfile num, the while loop will again
                                if(strcmp(command[j],"&")==0) break;//meet the background
                                if(strcmp(command[j],"<")==0) break;
                                if(strcmp(command[j],"<")!=0&&strcmp(command[j],"&")!=0) outfilenum++;
                                j++;
                            }
                        }
                        if(strcmp(command[i],"<")==0)
                        {
                            rein=i;
                            int j=i+1;
                            while(j<argu)
                            {
                                if(strcmp(command[j],"&")==0) break;//meet the background
                                if(strcmp(command[j],">")==0) break;
                                if(strcmp(command[j],">")!=0&&strcmp(command[j],"&")!=0) infilenum++;
                                j++;
                            }
                        }
                        if(strcmp(command[i],"|")==0) {pipepos=i;break;}
                        if(strcmp(command[i],"&")==0) {back=i;break;}
                        i++;
                    }
                    if(reout==0||rein==0) {linehist--;continue;} //no command after remove,</> is the first one
                    fflush(stdout);
                    if(pipepos!=-1)
                    {
                        fflush(stdout);
                        if(pipepos==0||pipepos==argu-1)//no command before | or after |
                        {write(STDERR_FILENO, error_message, strlen(error_message));continue;}
                        //extract 2 commands respectively
                        int index=0,index2=0;
                        int status;
                        int reid;
                        char* curcommand1[100];
                        char* curcommand2[100];
                        while(index<pipepos)
                        {
                            curcommand1[index]=strdup(command[index]);
                            index++;
                        }
                        curcommand1[index]=NULL;
                        int curcommand1len=index;
                        index=pipepos+1;
                        while(index<argu)
                        {
                            curcommand2[index2]=strdup(command[index]);
                            index++;
                            index2++;
                        }
                        curcommand2[index2]=NULL;
                        int curcommand2len=index2;
                        //connect pipe descriptor
                        int pipedes[2];
                        if(-1==pipe(pipedes))
                        {write(STDERR_FILENO, error_message, strlen(error_message));continue;};
                        fflush(stdout);
                        int firstson=fork();
                        if(firstson==-1)
                        {write(STDERR_FILENO, error_message, strlen(error_message));continue;};
                        if(firstson==0)
                        {   
                            if(-1==dup2(pipedes[1],STDOUT_FILENO))
                            {write(STDERR_FILENO, error_message, strlen(error_message));exit(0);};
                            close(pipedes[1]);//////////correct all
                            close(pipedes[0]);
                            if(-1==execvp(curcommand1[0],curcommand1))
                            {write(STDERR_FILENO, error_message, strlen(error_message));exit(0);};
                        }
                        fflush(stdout);
                        int secondson=fork();
                        if(secondson==-1)
                        {write(STDERR_FILENO, error_message, strlen(error_message));continue;};
                        if(secondson==0)
                        {
                            if(-1==dup2(pipedes[0],STDIN_FILENO))
                            {write(STDERR_FILENO, error_message, strlen(error_message));exit(0);};
                            close(pipedes[0]);
                            close(pipedes[1]);//////////correct all
                            if(-1==execvp(curcommand2[0],curcommand2))
                            {write(STDERR_FILENO, error_message, strlen(error_message));exit(0);};
                        }
                        //parent process do something
                        close(pipedes[0]);
                        close(pipedes[1]);
                        fflush(stdout);
                        while((reid=waitpid(-1,&status,0))>0)
                        {
                            if(reid==secondson)
                                {
                                    if(WIFEXITED(status))//second son has exited normally, first should also exited
                                        break;
                                }
                        }
                        if(reid==-1)
                        {write(STDERR_FILENO, error_message, strlen(error_message));continue;};
                        for(int d=0;d<curcommand1len;d++)free(curcommand1[d]);
                        for(int d=0;d<curcommand2len;d++)free(curcommand2[d]);
                    } 
                    // no redirection
                    else if(reout==-1&&rein==-1)
                    {
                        if(back==-1)
                        {
                            fflush(stdout);
                            int status;
                            int childid=fork();
                            //child process
                            if(childid==-1)
                            {write(STDERR_FILENO, error_message, strlen(error_message));continue;}
                            if(childid==0)
                            {
                                if(execvp(command[0],command)==-1)//detect the command can be executed
                                {
                                  write(STDERR_FILENO, error_message, strlen(error_message));
                                  exit(0);
                                }
                            }
                            else
                            { //-1 wait for any child process,now wait for certain childid
                              if(waitpid(childid,&status,0)==-1)
                              {write(STDERR_FILENO, error_message, strlen(error_message));continue;}
                            }
                        }
                        else
                        {//deal with background execution
                            fflush(stdout);
                            int status;
                            argu--;//remove the last & character
                            command[argu]=NULL;
                            int childid=fork();
                            //child process
                            if(childid==-1)
                            {write(STDERR_FILENO, error_message, strlen(error_message));continue;}
                            if(processnum<20)
                            processtable[processnum++]=childid;
                            if(childid==0)
                            {
                                if(execvp(command[0],command)==-1)//detect the command can be executed
                                {
                                  write(STDERR_FILENO, error_message, strlen(error_message));
                                  exit(0);
                                }
                            }
                        } 
                    } 
                    //exists redirection in,no out redirection
                    else if(reout==-1&&rein!=-1)
                    {
                        if(back==-1)
                        {
                            fflush(stdout);
                            if(rein==argu-1||infilenum!=1) //file numbers >1/or file come in
                            {write(STDERR_FILENO,error_message,strlen(error_message));continue;}
                            char* curcommand[100];
                            int index=0;//concatanate current command
                            while(index<100&&index<rein)
                            {
                                curcommand[index]=strdup(command[index]);
                                index++;
                            }
                            curcommand[index]=NULL;
                            fflush(stdout);
                            int status;
                            int childid=fork();
                            //child process
                            if(childid==-1)
                            {
                              write(STDERR_FILENO, error_message, strlen(error_message));
                              continue;
                            }
                            if(childid==0)
                            {
                                int fdin=open(command[rein+1],O_RDONLY);
                                if(fdin==-1)
                                {write(STDERR_FILENO, error_message, strlen(error_message));exit(0);}
                                if(dup2(fdin,STDIN_FILENO)==-1)
                                {write(STDERR_FILENO, error_message, strlen(error_message));exit(0);}
                                close(fdin);
                                if(execvp(curcommand[0],curcommand)==-1)//detect the command can be executed
                                {
                                  write(STDERR_FILENO, error_message, strlen(error_message));
                                  exit(0);
                                }
                            }
                            else
                            {
                              if(waitpid(childid,&status,0)==-1)
                              {write(STDERR_FILENO, error_message, strlen(error_message));continue;}
                            }
                            for(int d=0;d<index;d++)free(curcommand[d]);
                        }
                        else
                        {
                            fflush(stdout);
                            if(rein==argu-1||infilenum!=1) //file numbers >1/or file come in
                            {write(STDERR_FILENO,error_message,strlen(error_message));continue;}
                            char* curcommand[100];
                            int index=0;//concatanate current command
                            while(index<100&&index<rein)
                            {
                                curcommand[index]=strdup(command[index]);
                                index++;
                            }
                            curcommand[index]=NULL;//already remove those & characters
                            fflush(stdout);
                            int status;
                            int childid=fork();
                            //child process
                            if(childid==-1)
                            {
                              write(STDERR_FILENO, error_message, strlen(error_message));
                              continue;
                            }
                            if(processnum<20)
                            processtable[processnum++]=childid;
                            if(childid==0)
                            {
                                int fdin=open(command[rein+1],O_RDONLY);
                                if(fdin==-1)
                                {write(STDERR_FILENO, error_message, strlen(error_message));exit(0);}
                                if(dup2(fdin,STDIN_FILENO)==-1)
                                {write(STDERR_FILENO, error_message, strlen(error_message));exit(0);}
                                close(fdin);
                                if(execvp(curcommand[0],curcommand)==-1)//detect the command can be executed
                                {
                                  write(STDERR_FILENO, error_message, strlen(error_message));
                                  exit(0);
                                }
                            }
                            for(int d=0;d<index;d++)free(curcommand[d]);
                        }
                        
                    }
                    //exists redirection out,no in
                    else if(reout!=-1&&rein==-1)
                    {
                        if(back==-1)
                        {
                            fflush(stdout);
                            if(reout==argu-1||outfilenum!=1)//arguments num incorrect
                            {write(STDERR_FILENO,error_message,strlen(error_message));continue;}
                            char* curcommand[100];
                            int index=0;
                            while(index<reout)
                            {
                                curcommand[index]=strdup(command[index]);
                                //printf("%s\n",curcommand[index]);
                                index++;    
                            }
                            curcommand[index]=NULL;
                            fflush(stdout);
                            int status;
                            int childid=fork();          
                            //child process
                            if(childid==-1)
                            {
                              write(STDERR_FILENO, error_message, strlen(error_message));
                              continue;
                            }
                            if(childid==0)
                            {//file properties should be editable, different from the O_WRONLY&O_CREAT
                                int fdout=open(command[reout+1],O_CREAT|O_WRONLY|O_TRUNC,00744);
                                if(fdout==-1)
                                {write(STDERR_FILENO, error_message, strlen(error_message));exit(0);}
                                if(dup2(fdout,STDOUT_FILENO)==-1)
                                {write(STDERR_FILENO, error_message, strlen(error_message));exit(0);}   
                                close(fdout);
                                if(execvp(curcommand[0],curcommand)==-1)//detect the command can be executed
                                {
                                  write(STDERR_FILENO, error_message, strlen(error_message));
                                  exit(0);
                                }
                            }
                            else
                            {
                              if(waitpid(childid,&status,0)==-1)
                              {write(STDERR_FILENO, error_message, strlen(error_message));continue;}
                            }
                            for(int d=0;d<index;d++)free(curcommand[d]);
                        }
                        else
                        {
                            fflush(stdout);
                            if(reout==argu-1||outfilenum!=1)//arguments num incorrect
                            {write(STDERR_FILENO,error_message,strlen(error_message));continue;}
                            char* curcommand[100];
                            int index=0;
                            while(index<reout)
                            {
                                curcommand[index]=strdup(command[index]);
                                //printf("%s\n",curcommand[index]);
                                index++;    
                            }
                            curcommand[index]=NULL;
                            fflush(stdout);
                            int status;
                            int childid=fork();          
                            //child process
                            if(childid==-1)
                            {
                              write(STDERR_FILENO, error_message, strlen(error_message));
                              continue;
                            }
                            if(childid==0)
                            {
                                int fdout=open(command[reout+1],O_CREAT|O_WRONLY|O_TRUNC,00744);
                                if(fdout==-1)
                                {write(STDERR_FILENO, error_message, strlen(error_message));exit(0);}
                                if(dup2(fdout,STDOUT_FILENO)==-1)
                                {write(STDERR_FILENO, error_message, strlen(error_message));exit(0);}   
                                close(fdout);
                                if(execvp(curcommand[0],curcommand)==-1)//detect the command can be executed
                                {
                                  write(STDERR_FILENO, error_message, strlen(error_message));
                                  exit(0);
                                }
                            }
                            for(int d=0;d<index;d++)free(curcommand[d]);
                        }
                    }
                    //both redirection out && redirection in
                    else if(reout!=-1&&rein!=-1)
                    {
                        if(back==-1)
                        {
                            fflush(stdout);
                            if(reout==argu-1||rein==argu-1||outfilenum>1||infilenum>1)
                            {write(STDERR_FILENO,error_message,strlen(error_message));continue;}
                            char* curcommand[100];
                            int index=0;
                            while(index<reout&&index<rein)
                            {
                                curcommand[index]=strdup(command[index]);
                                //printf("%s\n",curcommand[index]);
                                index++;
                            }
                            curcommand[index]=NULL;
                            fflush(stdout);
                            int status;
                            int childid=fork();
                            //child process
                            if(childid==0)
                            {
                                int fdout=open(command[reout+1],O_WRONLY|O_CREAT|O_TRUNC,00744);
                                int fdin=open(command[rein+1],O_RDONLY);
                                if(fdout==-1||fdin==-1)
                                {write(STDERR_FILENO, error_message, strlen(error_message));exit(0);} 
                                if(dup2(fdout,STDOUT_FILENO)==-1||dup2(fdin,STDIN_FILENO)==-1)
                                {write(STDERR_FILENO, error_message, strlen(error_message));exit(0);}
                                close(fdout);
                                close(fdin);
                                if(execvp(curcommand[0],curcommand)==-1)
                                {
                                    write(STDERR_FILENO, error_message, strlen(error_message));
                                    exit(0);                       
                                }
                            }
                            else
                            {
                                if(waitpid(childid,&status,0)==-1)
                                {write(STDERR_FILENO, error_message, strlen(error_message));continue;}
                            }
                            for(int d=0;d<index;d++)free(curcommand[d]);
                        }
                        else
                        {  //deal with background execution
                            fflush(stdout);
                            if(reout==argu-1||rein==argu-1||outfilenum>1||infilenum>1)
                            {write(STDERR_FILENO,error_message,strlen(error_message));exit(0);}
                            char* curcommand[100];
                            int index=0;
                            while(index<reout&&index<rein)
                            {
                                curcommand[index]=strdup(command[index]);
                                index++;
                            }
                            curcommand[index]=NULL;
                            fflush(stdout);
                            int status;
                            int childid=fork();
                            //child process
                            //record the process in the processtable
                            if(processnum<20)
                            processtable[processnum++]=childid;
                            if(childid==0)
                            {
                                int fdout=open(command[reout+1],O_WRONLY|O_CREAT|O_TRUNC,00744);
                                int fdin=open(command[rein+1],O_RDONLY);
                                if(fdout==-1||fdin==-1)
                                {write(STDERR_FILENO, error_message, strlen(error_message));exit(0);} 
                                if(dup2(fdout,STDOUT_FILENO)==-1||dup2(fdin,STDIN_FILENO)==-1)
                                {write(STDERR_FILENO, error_message, strlen(error_message));exit(0);}
                                close(fdout);
                                close(fdin);
                                if(execvp(curcommand[0],curcommand)==-1)
                                {
                                    write(STDERR_FILENO, error_message, strlen(error_message));
                                    exit(0);                        
                                }
                            }
                            for(int d=0;d<index;d++)free(curcommand[d]);
                        }
                      
                    }
                    
                }
                for(int p=0;p<argu;p++) free(command[p]);
            }
        }
    }
    return 0;
}
