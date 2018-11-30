#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

#include "file_explorer.h"

//current list of methods
void init_fs(FILE **fp, struct fs_attr *fs);
int open_file(FILE **fp, char *name[]);
void set_directory(FILE **fp, int* filep, struct fat32_entry dir[]);
void ls(FILE **fp, struct fat32_entry dir[]);
void execute_command(char *command[], FILE **fp, int *filep, struct fs_attr *fs, struct fat32_entry dir[]);
void pwd();
void cd(char* directoryName, FILE **fp, struct fat32_entry dir[], int *filePointer, struct fs_attr *info);
int find_address(char *name, struct fat32_entry dir[], struct fs_attr *fs);
int LABtoOffset(unsigned int sector, struct fs_attr *fs);
//I haven't had a chance to test these last two functions yet, they may need editing
//Use these last two for implement the 'cd' change directory command

//will need to update this string when the 'cd' command is run
char* current_directory = "/";
int root = 1;

void init_fs(FILE **fp, struct fs_attr *fs){ //used to initialize the information of the file system we're working with
	//start with bytes_per_sector
	unsigned short bytes;
	fseek(*fp, 11, SEEK_SET); //point to fp object, and with an offset of 11, or any other offset defined below,
				//using SEEK_SET which tells the function to start with beginning of file,
				//seek the given information
	fread(&bytes, 1, 2, *fp); //then, read the block of data from the stream into the value of bytes, or any other
				//substituted variable defined below
	fs->bytes_per_sector = bytes; //finally, store this value into their respective variable in the struct fs_attr

	//Because the boot sector is sector 0, we can use the offsets for each variable below as defined as in the Microsoft white
	//paper pdf, or any other resource on FAT32 online, to determine where the respective information lies in the .img file
	//(#sectors * sector size + offset = 0 * size + offset = offset).

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

	//printf("%d\n",fs->fat_size); //debugging
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
                fread(&dir[i],1,32,*fp); //see init_fs for info on fread()
                *filep += 32; //increment by 32 because we're in FAT32
        }
}

void ls(FILE **fp, struct fat32_entry dir[]){ //list contents of directory
	int i;
	//printf(" first entry: %s\n",dir[0].filename);
	for(i = 0; i < 16; i++){
		//only display files with attributes 0x01, 0x10, 0x20, or 0x30 (1,16,32, and 48 in decimal)
		if(dir[i].attributes == 1 || dir[i].attributes == 16 || dir[i].attributes == 32 || dir[i].attributes == 48){
			printf("%.*s\n",11,dir[i].filename); //print filename and extension (combined into filename)
		}
	}
}

void pwd(){ //print working directory
	if(root){
		printf("%s\n",current_directory);
	}else{
		printf("%.*s\n",(int)strlen(current_directory),current_directory + 1);
	}
}

//change directory function
//this allows the user to change directories using the cd command
void cd(char* directoryName, FILE **fp, struct fat32_entry dir[], int *filePointer, struct fs_attr *info)
{
	if(directoryName == NULL) //if cd command is not followed by a directory, go back to root directory
		return;

//converting to upper case
	int i = 0;
	while(directoryName[i] != '\0')
	{
		directoryName[i] = toupper(directoryName[i]);
		i++;
	}

	i = 0;//where we are in the string
	int j = 0;//the start index of the directory name in the string
	int dirLength = 0;//the length of the directory name

	while(directoryName[i] != '\0'){
		if (directoryName[i] == '/'){
			char temp [dirLength+1];
			strncpy(temp,directoryName + j, i-j);
			temp[dirLength] = '\0';
			j = i + 1;
			dirLength = 0;
			i++;

		*filePointer = find_address(temp,dir, &(*info));

		if(*filePointer == -1){
			printf("cd: %s: No such file or directory\n", directoryName);
			return;
		}
		//directory exits, so reset pwd() value to new directory
		char* temp2;
		if(strcmp(temp,"..") != 0){ //if it does not equal ..
			root = 0;
			temp2 = malloc(strlen(current_directory)+strlen(temp)+1);
			temp2[0] = '\0'; //empty array
			strcat(temp2,current_directory); //copy current_directory
			strcat(temp2,"/");
			strcat(temp2,temp); //plus new directory, to get full path
			current_directory = temp2; //reset pointer to this character array
		}else if(strcmp(current_directory,"/") != 0){ //otherwise we're dealing with a .., and not at root
			/*char* temp3;
			char** temp4;
			temp2 = malloc(strlen(current_directory)+strlen(temp)+1);
			temp2[0] = '\0';
			strcat(temp2,"/"); //root slash
			int c = 0;
			printf("here\n");
			while((temp3 = strsep(&current_directory, "/")) != NULL){
				printf("%s\n",temp3);
				&(temp4[c]) = malloc(strlen(current_directory)+strlen(temp)+1);
				&(temp4[c]) = '\0';
				strcat(&(temp4[c]),current_directory);
				strcat(&(temp4[c]),"/");
				c++;
			}
			printf("here2\n");
			int index = 0;
			for(index = 0; index < c-1; index++){
				strcat(temp2,&(temp4[index]));
			}*/
			temp2 = malloc(strlen(current_directory));
			temp2[0] = '\0';
			int length = ((int)strlen(current_directory) - 8);
			if(length > 7){
				strncpy(temp2, current_directory, length);
				root = 0;
			}else{
				temp2 = "/";
				root = 1;
			}
			current_directory = temp2;
		}
		//reassign file pointer
		fseek(*fp, *filePointer, SEEK_SET);
		set_directory(&(*fp),&(*filePointer),dir);
		continue;
	}
	dirLength++;
	i++;
	}

	char temp[dirLength+1];
	strncpy(temp,directoryName + j, (i+1)-j);
	*filePointer = find_address(temp,dir,&(*info));
	if(*filePointer == -1){
		printf("cd: %s: No such file or directory\n", directoryName);
		return;
	}
	char* temp2; //same code repeated from above
	if(strcmp(temp,"..") != 0){ //if it doesn't equal ..
		root = 0;
		temp2 = malloc(strlen(current_directory)+strlen(temp)+1);
		temp2[0] = '\0'; //empty array
		strcat(temp2,current_directory); //copy current_directory
		strcat(temp2,"/");
		strcat(temp2,temp); //plus new directory, to get full path
		current_directory = temp2; //reset pointer to this character array
	}
	fseek(*fp, *filePointer, SEEK_SET);
	set_directory(&(*fp),&(*filePointer),dir);
}

int LBAtoOffset(unsigned int sector, struct fs_attr *fs){ //use to translate logical block address to offseted address usable for the find_address() method
        return ((sector-2)*fs->bytes_per_sector) + (fs->bytes_per_sector * fs->reserved_sector_count) + (fs->num_fats * fs->fat_size * fs->bytes_per_sector); //similar to above, just adding offset of (sector-2) * it's size in bytes
}


int find_address(char *name, struct fat32_entry dir[], struct fs_attr *fs){ //find cluster address of directory
	int i, j;
	int address = -1; //we will store the address in this variable
	for(i = 0; i < 16; i++){ //number of directory records is 16, that's why that's there
		char *dir_name; //used to copy directory name and compare it below to check we found the correct directory
		j = 0;
		while(dir[i].filename[j] != '\0' && j < 13){ //'\0' represents end of filename, directonry entry i at index j in name
			if(dir[i].filename[j] == ' '){
				dir_name = (char*)malloc(sizeof(char)*j); //allocate memory for the dir_name variable
				strncpy(dir_name,dir[i].filename,j); //copy j entries of the filename into the variable dir_name
				break; //break from the loop to continue below
			}
			j++; //iterate
		}
		if(strcmp(dir_name,name) == 0){ //if the correct directory specified by the user is found
			address = LBAtoOffset(dir[i].first_cluster,&(*fs)); //get actual address
			if(dir[i].first_cluster == 0){ //if the first cluster is zero, manually assignt he address
				address = (fs->num_fats * fs->fat_size * fs->bytes_per_sector) + (fs->reserved_sector_count * fs->bytes_per_sector);
			}
			break; //break from the for loop
		}	
	}
	return address; //if -1 is returned the directory was not found
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
		set_directory(&(*fp),&(*filep),dir);
	}else if(strcmp(command[0],"close") == 0){ //close the opened file
		if(*fp == NULL){
			printf("No image file is open\n");
		}else{
			*fp = NULL; //set file pointer to null, eg. close the file
			char* temp = "/";
			current_directory = temp;
			root = 1;
		}
	}else if(strcmp(command[0],"ls") == 0){ //list contents of directory
                if(*fp == NULL){
                        printf("Must open the image file first\n");
                }else{
                        ls(&(*fp),dir); //list file directory
                }
        }else if(strcmp(command[0],"pwd") == 0){
		if(*fp == NULL){
			printf("Must open the image file first\n");
		}else{
			pwd();
		}
	}else if(strcmp(command[0],"cd") == 0){
		if(*fp == NULL){
			printf("Must open the image file first\n");
		}else{
			cd(command[1], &(*fp), dir, &(*filep), &(*fs));
		}
	}else{
		printf("Pleas enter a supported command: open, close, ls, pwd, cd, exit\n"); //list of supported commands, update as you add more
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
