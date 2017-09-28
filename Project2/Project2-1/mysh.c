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
        if(fgets(input,128,stdin)==NULL) {
            write(STDERR_FILENO, error_message, strlen(error_message));
            break;
        } else {
            //detect the input line length exceeds 128 bytes
            if (input[sizeof input - 1] == '\0' && input[sizeof input - 2] != '\n') {
                write(STDERR_FILENO, error_message, strlen(error_message));
                continue;
            } else {
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
                while((token=strtok_r(NULL," \t",&reserve))) {
                    command[argu]=strdup(token);
                    argu++;
                }
                //printf("%c",command[i-1][strlen(command[i-1]-1)]);
                //printf("%s",command[i-1]);
                command[argu]=NULL;
                argu--;
                //printf("%s",command[argu-1]);
//////////////////////////////////////////////////////////////////////////
                //parse command already
                //deal with cd
                if(strcmp(command[0],"cd")==0) {
                    if(command[1]==NULL) {
                        char* homepath=getenv("HOME");
                        if(homepath==NULL) {
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            continue;
                        }
                        if(chdir(homepath)==0) continue;//success cd into home
                        else{write(STDERR_FILENO, error_message, strlen(error_message));}
                    } else {
                        if(chdir(command[1])==0) continue;
                        else{write(STDERR_FILENO, error_message, strlen(error_message));}
                    }   
                //deal with pwd
                } else if(strcmp(command[0],"pwd")==0) {  
                    char* bufferpath = malloc(128);
                    if(bufferpath==NULL)
                        write(STDERR_FILENO, error_message, strlen(error_message));
                    if(getcwd(bufferpath,sizeof(bufferpath))==NULL)
                        write(STDERR_FILENO, error_message, strlen(error_message));
                    printf("%s\n",bufferpath);
                //deal with exit
                } else if(strcmp(command[0],"exit")) {

                } else {    //else if(strcmp(command[argu-1],"<")) {
                    //Here deal with all cases that is not built-in methods
                    int childid=fork();
                    //child process 
                    if(childid==0) {
                        int fd=open(command[argu],O_WRONLY|O_CREAT|O_TRUNC);
                        if(fd==-1){
                            write(STDERR_FILENO, error_message, strlen(error_message));
                            continue;
                        }
                        dup2(STDOUT_FILENO,fd);
                        execvp(command[0],)
                        close(fd);
                    }
                }
                
                    
                    
                }

            }
        }
    }
    return 0;
}