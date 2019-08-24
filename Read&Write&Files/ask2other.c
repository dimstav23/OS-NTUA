#include <stdio.h> //include the necessary libraries
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifndef BUF_SIZE
#define BUF_SIZE 1024 //if buf size is not defined, give it the value of 1024
#endif
//char buf[BUF_SIZE];

void doWrite(int fd, const char *buff, int len)
{	//lseek(fd,-29,SEEK_END);
	int write_num = write(fd,buff,len);
	if (write_num == -1 ){
		perror("Unable to write\n");
		exit(1);
	}
	if (write_num == 0) 
		printf("for some reason we didn't achieve in writing. (will never happen though)\n");	//in case our whole len is not written continue till the end	
	if (write_num < len )
		doWrite(fd,buff+write_num,len-write_num);
}          

void write_file(int fd, const char *infile)
{	
	//open the file
	int fdin = open (infile,O_RDWR);
	if (fdin == -1){
		perror("Unable to open file\n");
		exit(1);
	}
	//read from the file with fd = fdin
	char buf[BUF_SIZE];
	ssize_t ret=read(fdin,buf,BUF_SIZE); //int
	//int sum;//counts how many characters function "read" sends successfully to our buffer. 
	//sum=ret;
	if (ret == -1) {
	    	perror("Unable to read file\n");
     		exit(1);
	}
	else if (ret == 0) {
		printf("We have reached EOF, our file is empty\n");
		//exit(1);
	}
	else{	
		doWrite(fd,buf,ret);
		while(ret>0){
			ret=read(fdin,buf/*+ret*/,BUF_SIZE/*-ret*/); //successfully read a complete file
			//printf("%d\n",ret);
			//printf("%s\n",ret);
			//if (ret==0) printf("%d\n",strlen(buf));
			//sum+=ret; if i want to write after reading whole file
			if (ret == -1){
				 printf("Problem with reading the whole file \n");
				 exit(1);
			}
			else if (ret == 0 )
				break;
			doWrite(fd,buf,ret);
		}
		close(fdin);	
	} 
      	
	//doWrite(fd,buf,sum); if i want to write after reading the whole file 
}

void OutCreate(char *infile1,char *infile2,char *outfile)
{
	int openFlags;
	//set up flags
	
	if (outfile==NULL){
		openFlags = O_CREAT | O_RDWR | O_TRUNC|  O_APPEND;
	}
	else if ((strcmp(infile1,outfile)==0)/*||(strcmp(infile2,outfile)==0)*/){
		openFlags = O_CREAT | O_RDWR |  O_APPEND;
	}
	else if ((strcmp(infile2,outfile)==0)){
		openFlags = O_RDWR ;
	}
	else{
		openFlags = O_CREAT | O_RDWR | O_TRUNC|  O_APPEND;// | O_APPEND;
        }
	mode_t filePerms = S_IRUSR | S_IWUSR ;  //| S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH ; /* rw-rw-rw- */
	int outputfd;	
	if (outfile == NULL){
		outputfd = open("fconc.out",openFlags,filePerms);
		if (outputfd == -1){
			perror("Cannot create file \n");
			exit(1);
		}
		write_file(outputfd,infile1);
		write_file(outputfd,infile2);
	}
	else{
		outputfd = open(outfile,openFlags,filePerms);
		if (outputfd == -1){
                        perror("Cannot create file \n");
                        exit(1);
		}
		if (strcmp(infile1,outfile)==0)
			write_file(outputfd,infile2);
		else if (strcmp(infile2,outfile)==0)
			write_file(outputfd,infile1);
		else{
			write_file(outputfd,infile1);
			//printf("%d\n",outputfd);
			write_file(outputfd,infile2);
		}
	}
	//getchar();
	close(outputfd);	
}

int main(int argc,char *argv[])
{
	int inputFd1,inputFd2;
	//ssize_t numRead;
	//char buf[BUF_SIZE];
	//check for valid syntax of fconc
	if (argc<3 || argc>=5)
	{
		printf("Usage: ./fconc infile1 infile2 [outfile (default:fconc.out)]\n");
		exit(1);
	}
	else if (strcmp(argv[1],"--help")==0) //|| strcmp(argv[2],"--help")==0) //check if user needs help 
	{
		printf("%s InputFile1 InputFile2 OutputFile(not necessarily)\n",argv[0]);
		exit(1);
	}
	//check if the first two files can be opened	
	inputFd1 = open(argv[1],O_RDWR);
	//printf("%d\n",inputFd1);
	if (inputFd1 == -1)
	{
		printf("file %s does not exist\n",argv[1]);
		exit(1);
	}
	
	inputFd2 = open(argv[2],O_RDWR);
	//printf("%d\n",inputFd2);
	if (inputFd2 == -1)
	{
		printf("file %s does not exist\n",argv[2]);
		exit(1);
	}
		
	if (argc == 3)
		OutCreate(argv[1],argv[2],NULL);
	else{ 
		if (strcmp(argv[1],argv[3])==0){
			close(inputFd1);
			OutCreate(argv[1],argv[2],argv[3]);
		}
		else if (strcmp(argv[2],argv[3])==0){
			OutCreate(argv[1],argv[2],NULL);
			write_file(inputFd2,"fconc.out");
			remove("fconc.out");			
		}
		else
		OutCreate(argv[1],argv[2],argv[3]);
	}	
return 0;
}
