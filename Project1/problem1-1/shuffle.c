//@ Huawei Wang 9077759414
//final version read once, alternate write to files

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
int main(int argc,char*argv[])
{
	char *inputfile = NULL;
	char *outputfile = NULL;
	int c;
	//deal with the command line
	opterr = 0;
  while ((c = getopt (argc, argv, "i:o:")) != -1)
	  switch (c)
		{
				case 'i':
				if(optarg==0)
				{
					fprintf (stderr,"Usage: shuffle -i inputfile -o outputfile\n");
				 	exit(1);
				}
				inputfile = strdup(optarg);
				break;
		    case 'o':
				if(optarg==0)
				{
					fprintf (stderr,"Usage: shuffle -i inputfile -o outputfile\n");
				 	exit(1);
				}
				outputfile = strdup(optarg);
				break;
				case '?':
				{
					fprintf (stderr,"Usage: shuffle -i inputfile -o outputfile\n");
					exit(1);
				}
				default:
				{
					fprintf (stderr,"Usage: shuffle -i inputfile -o outputfile\n");
				 	exit(1);
				}
		 	}
			if(inputfile==NULL||outputfile==NULL)
			{
				fprintf (stderr,"Usage: shuffle -i inputfile -o outputfile\n");
				exit(1);
			}
	//input files
	FILE* inputpointer;
	inputpointer = fopen(inputfile,"r");
	if(inputpointer == NULL)
	{
		fprintf(stderr,"Error: Cannot open file %s\n",inputfile);
		exit(1);
	}
	//printf("%s\n",inputfile);
	//printf("%s\n",outputfile);
	//calculate the total size of the input file
	struct stat container;
	if(stat(inputfile,&container) == -1)
	{
		fprintf(stderr,"Error stat\n");
		exit(1);
	}
	//printf("%d\n",(int)container.st_size);
	//read into a total pointer named readtotal
	char *readtotal = malloc(container.st_size);
	if(readtotal == NULL)
	{
		free(readtotal);
		exit(1);
	}
	int fr = fread(readtotal,container.st_size,1,inputpointer);
	if(ferror(inputpointer)>0||fr==0)
	{
		fprintf(stderr,"Error read\n");
		clearerr(inputpointer);
		exit(1);
	}
	if(feof(inputpointer)>0||fr==0)
	{
		clearerr(inputpointer);
		fclose(inputpointer);
	}
	//printf("%c\n",readtotal[strlen(readtotal)-2]);
////////////////////////////////////////////////////////
//shuffle and output to the file
//change turns around
	char* change = malloc(512);
	int turn = 1;
	int i = 0,j = strlen(readtotal)-1;
	int curcount = 0;
	FILE* outputpointer;
	outputpointer = fopen(outputfile,"w");
	if(outputpointer == NULL)
	{
		fprintf(stderr,"Error: Cannot create file %s\n",outputfile);
		exit(1);
	}
	fclose(outputpointer);
	outputpointer = fopen(outputfile,"a");
	if(outputpointer == NULL)
	{
		fprintf(stderr,"Error: Cannot open file %s\n",outputfile);
		exit(1);
	}
	while(j>=i)
	{
		curcount = i;
		if(turn == 1)
		{
			while(readtotal[curcount]!='\n')
			{
				curcount++;
			}
			memcpy(change,&readtotal[i],curcount-i+1);
			i = curcount+1;
			size_t temp = fwrite(change,1,strlen(change),outputpointer);
			if(temp == 0)
			{
				fprintf(stderr,"Failed to write\n");
				exit(1);
			}
			curcount=0;
			memset(change,0,strlen(change));
			turn = turn*(-1);

		}
		else if(turn == -1)
		{
			curcount = j-1;
			while(readtotal[curcount]!='\n')
			{
				curcount--;
			}
			curcount++;
			memcpy(change,&readtotal[curcount],j-curcount+1);
			j = curcount-1;
			size_t temp = fwrite(change,1,strlen(change),outputpointer);
			if(temp == 0)
			{
				fprintf(stderr,"Failed to write\n");
				exit(1);
			}
			curcount=0;
			memset(change,0,strlen(change));
			turn = turn*(-1);
		}
	}
	free(change);
	free(readtotal);
	fclose(outputpointer);
	return 0;
}
