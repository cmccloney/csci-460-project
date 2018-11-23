#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "file_explorer.h"

void init_fs(FILE **fp, struct fs_attr *fs){ //used to initialize the information of the file system we're working with
	//start with bytes_per_sector
	unsigned short bytes;
	fseek(*fp, 11, SEEK_SET); //point to fp object, and with an offset of 11, or any other offset defined below,
				//using SEEK_SET which tells the function to start with beginning of file,
				//seek the given information
	fread(&bytes, 1, 2, *fp); //then, read the block of data from the stream into the value of bytes, or any other
				//substituted variable defined below
	fs->bytes_per_sector = bytes; //finally, store this value into their respective variable in the struct fs_attr

	//next onto sectors_per_cluster, where we complete a similar operation
	unsigned char secs;
	fseek(*fp, 13, SEEK_SET);
	fread(&secs, 1, 1, *fp);
	fs->sectors_per_cluster = secs;

	//reserved_sectors
	unsigned short reserved;
	fseek(*fp, 14, SEEK_SET);
	fread(&reserved, 1, 2, *fp);
	fs->reserved_sector_count = reserved;

	//fat count
	unsigned char fats;
	fseek(*fp, 16, SEEK_SET);
	fread(&fats, 1, 1, *fp);
	fs->num_fats = fats;

	//fat size
	unsigned int size;
	fseek(*fp, 36, SEEK_SET);
	fread(&size, 1, 4, *fp);
	fs->fat_size = size;

	//label name
	char name[11];
	fseek(*fp, 71, SEEK_SET);
	fread(&name, 1, 11, *fp);
	strcpy(fs->label,name); //copy string over
}

int open_file(FILE **fp, char *name[]){ //used to open file
	struct stat file_checker;
	if(*fp != NULL){
		printf("File already open\n"); //the file is already open if *fp is not null
		return 0; //return 0 for error
	}else if(stat(name[1],&file_checker) == -1){ //use this stat struct to check if the file can be found,
						//this 'stat' is used for determining the type of a file, however
						//if the file can't be found, it will return -1, making it useful in this case
		printf("File not found\n"); //the file couldn't be found
		return 0;
	}else{ //else, we can open the file
		*fp = fopen(name[1],"r"); //open the file for reading use only
		printf("Successfully opened %s\n",name[1]);
		//printf("Here are the contents of the file %s:\n",name[1]);
		//int c;
		//while ((c = getc(*fp)) != EOF){ //while we still haven't reached the end of the file,
		//	putchar(c); //print out character by character
		//}
		//fclose(*fp); //close the file after reading contents
		return 1; //return 1 if successfully completed
	}
}

void set_directory(FILE **fp, int* filep, struct fat32_entry dir[]){ //set new directory
	int i;
	for(i = 0; i < 16; i++){ //loop through all 16 entries of directory array, reading in values from fp in order to set the new directory
		printf("before : %s\n",dir[i].filename);
		fread(&dir[i],1,32,*fp); //see init_fs for info on fread()
		//I don't think fread works, need to fix
		//printf("Here are the contents of the file %s:\n",name[1]);
                int c;
                while ((c = getc(*fp)) != EOF){ //while we still haven't reached the end of the file,
                      putchar(c); //print out character by character
                }
                //fclose(*fp); //close the file after reading contents
		printf("after : %s\n",dir[i].filename);
		//*filep += 32; //increment by 32 because we're in FAT32
	}
}

void execute_command(char *command[], FILE **fp, int *filep, struct fs_attr *fs, struct fat32_entry dir[]){ //execute command enterd by user
	//char temp[]; //might put command into temp, and implement switch statement instead of if-else chain
	if(strcmp(command[0],"open") == 0){ //if the command entered is 'open'
		if(open_file(&(*fp),command) == 0){ //if the file failed to open (returned zero)
			return;
		}
		init_fs(&(*fp),&(*fs)); //initialize file system
		//set up file pointer (# Fats * size * # bytes) + (# reserved sectors * # bytes per reserved sector)
		*filep = (fs->num_fats * fs->fat_size * fs->bytes_per_sector) + (fs->reserved_sector_count * fs->bytes_per_sector);
		fseek(*fp, *filep, SEEK_SET); //see init_fs for info. on fseek()
		set_directory(&(*fp),&(*filep),dir); //set new directory
	}else if(strcmp(command[0],"close") == 0){ //close the opened file
		if(*fp == NULL){
			printf("No file is open\n");
		}else{
			*fp = NULL; //set file pointer to null, eg. close the file
		}
	}else if(strcmp(command[0],"ls") == 0){ //list contents of directory
		if(*fp == NULL){
			printf("Must open the file first\n");
		}else{
			//ls(&(*fp),dir); //list file directory
		}
	}else{
		printf("Pleas enter a supported command: open, close, exit\n"); //list of supported commands, update as you add more
	}
}

int main(){
	struct fs_attr fs;
	struct fat32_entry dir[16];
	char* command = (char*) malloc(512); //set max command size as 512, more than large enough for this small implementation
	int filep = 0; //int file pointer, necessary for incrementing through file by bit, see set_directory()
	FILE *fp = NULL; //initialized empty file pointer, passed to execute_command() below
	int running = 1;
	while(running){ //infinite loop that keeps the command prompt open
		printf("Enter command> "); //prompt for command
		while( fgets(command, 512, stdin) == 0){} //while the user has not inputted anything, loop infinitely
		int arg_count = 0; //count number of arguments inputted
		char *args[10]; //max of ten args, can increase, but 10 is more than enough for this project I think
		char *arg_p; //used in splitting command below
		char *com_copy = strdup(command); //duplicate command, otherwise our program will fail after 2 or more commands
		while(((arg_p = strsep(&com_copy, " \t\n")) != NULL) && arg_count < 10){ //seperate non-null command on spaces, tabs, and, 
			//return lines, and make sure the number of arguments is less than ten, do this
			args[arg_count] = strndup(arg_p, 512); //returns pointer to copied string
			arg_count++;
			
		}
		if(strcmp(args[0],"exit") == 0){ //exit the file reader
			return 0;
		} else if(args[0] != NULL){ //if there is a command to be executed
			execute_command(args,&fp,&filep,&fs,dir); //execute it
		}
	}
	return 0;
}
