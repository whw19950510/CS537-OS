#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include <stdbool.h>
int main(int argc,char* argv[])
{
    char error_message[30] = "An error has occurred\n";
    int linehist=1; 
    bool finish=false;
    while(!finish)
    {
        printf("mysh (%d)> ",linehist);
        char input[128];
        input[sizeof input - 1] = '1';//set last character to non '\0
        //no characters are founded or end of file occurs
        if(fgets(input,128,stdin)==NULL)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            break;
        }
        else
        {
            //detect the input line length exceeds 128 bytes
            if (input[sizeof input - 1] == '\0' && input[sizeof input - 2] != '\n')
            {
                write(STDERR_FILENO, error_message, strlen(error_message));
                continue;
            }
            else
            {
                input[strlen(input)-1]='\0';//concatate the \n, prevent execute
                char* command[128];
                char* todeal = input;
                char* reserve; 
                int argu=0;                 //number of arguments in the command,\n removed
                char* token = strtok_r(todeal," \t",&reserve);
                if(token==NULL) continue;
                //printf("%s",token);
                //printf("token:%s\n",command[0]);
                else {command[argu++]=strdup(token);} 
                while((token=strtok_r(NULL," \t",&reserve)))
                {
                    command[argu]=strdup(token);
                    argu++;
                }
                //printf("%c",command[i-1][strlen(command[i-1]-1)]);
                //printf("%s",command[i-1]);
                command[argu]=NULL;
                //printf("%s",command[argu-1]);
//////////////////////////////////////////////////////////////////////////
                //parse command already
                //deal with cd
                if(strcmp(command[0],"cd")==0)
                {
                    if(command[1]==NULL)
                    {
                        char* homepath=getenv("HOME");
                        if(homepath==NULL)
                        {
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            continue;
                        }
                        if(chdir(homepath)==0) continue;//success cd into home
                        else{write(STDERR_FILENO, error_message, strlen(error_message));}
                    }
                    else
                    {
                        if(chdir(command[1])==0) continue;
                        else{write(STDERR_FILENO, error_message, strlen(error_message));}
                    }   
                //deal with pwd
                }
                else if(strcmp(command[0],"pwd")==0)
                {  
                    char* bufferpath = malloc(128);
                    if(bufferpath==NULL)
                        write(STDERR_FILENO, error_message, strlen(error_message));
                    if(getcwd(bufferpath,sizeof(bufferpath))==NULL)
                        write(STDERR_FILENO, error_message, strlen(error_message));
                    printf("%s\n",bufferpath);
                //deal with exit
                }
                else if(strcmp(command[0],"exit"))
                {

                }
                else
                {    
                    //Here deal with all cases that is not built-in methods
                    int reout=-1,rein=-1,pipepos=-1,back=-1;
                    int outfilenum=0,infilenum=0;
                    int i=0;
                    while(i<=argu)
                    {
                        if(strcmp(command[i],">")==0)
                        {
                            reout=i;
                            //extract outfile number
                            int j=i+1;
                            while(j<=argu) 
                            {
                                if(strcmp(command[j],"&")==0) break;//meet the background
                                if(command[j]!=NULL&&strcmp(command[j],"<")!=0) outfilenum++;
                            }
                        }
                        if(strcmp(command[i],"<")==0)
                        {
                            rein=i;
                            int j=i+1;
                            while(j<=argu) 
                            {
                                if(strcmp(command[j],"&")==0) break;//meet the background
                                if(command[j]!=NULL&&strcmp(command[j],">")!=0) infilenum++;
                            }
                        }
                        if(strcmp(command[i],"|")==0) pipepos=i;
                        if(strcmp(command[i],"&")==0) back=i;
                        i++;
                    }
                    if(reout==0||rein==0) continue; //no command after remove  //no arguments cases detected
                    if(rein+1==reout||reout+1==rein||reout==argu||rein==argu||outfilenum==0||infilenum==0)//no <> will be tested
                    {write(STDERR_FILENO,error_message,sizeof(error_message));continue;}
                    else if(reout!=-1&&rein!=-1)
                    {   //exists output redirection&&input redirection
                        if(outfilenum>1||infilenum>1)
                        {write(STDERR_FILENO,error_message,sizeof(error_message));continue;}
                        int childid=fork();
                        //child process 
                        if(childid==0)
                        {
                            int fdo=open(command[reout+1],O_WRONLY|O_CREAT|O_TRUNC);
                            int fdi=open(command[rein+1],O_RDONLY);
                            if(fdo==-1||fdi==-1)
                            {
                                write(STDERR_FILENO, error_message, strlen(error_message));
                                continue;
                            }
                            if(dup2(fdo,1)==-1||dup2(fdi,0)==-1)
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            close(fdo);
                            close(fdi);
                            execvp(command[0],command);
                            
                        }
                        continue;
                    }
                    else if(reout==-1&&rein!=-1)//exists redirection in,no out
                    {
                        if(outfilenum>1||infilenum>1)
                        {write(STDERR_FILENO,error_message,sizeof(error_message));continue;}
                    }
                    else if(reout!=-1&&rein==-1)//exists redirection out,no in
                    {
                        if(outfilenum>1||infilenum>1)
                        {write(STDERR_FILENO,error_message,sizeof(error_message));continue;}
                    }
                    
                }                        
            }
        }
    }
    return 0;
}