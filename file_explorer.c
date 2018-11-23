#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

struct fat32_entry{ //directory entry
	char filename[8];
	char extension[3];
	unsigned char attributes;
	unsigned char reserved;
	unsigned char creation_ms;
	unsigned short creation_time;
	unsigned short creation_date;
	unsigned short last_access_time;
	unsigned short cluster_hi;
	unsigned short modified_time;
	unsigned short modified_date;
	unsigned short first_cluster;
	unsigned int file_size;
}__attribute__((__packed__)); //this keyword makes the struct take the smallest amount of possible space in memory
			     //or cram variables together without padding, which is important for fat entries,
			     //and we're not too worried about overflows with these entries, in this project

struct fs_attr{ //information about the file system itself
 	unsigned short bytes_per_sector;
  	unsigned char sectors_per_cluster;
  	unsigned char num_fats;
  	unsigned short reserved_sector_count;
  	unsigned int fat_size;
  	char label[11];
};

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

int open_file(FILE **fp, char *token[]){ //used to open file
	struct stat file_checker;
	if(*fp != NULL){
		printf("File already open\n"); //the file is already open if *fp is not null
		return 0; //return 0 for error
	}else if(stat(token[1],&file_checker) == -1){ //use this stat struct to check if the file can be found,
						//this 'stat' is used for determining the type of a file, however
						//if the file can't be found, it will return -1, making it useful in this case
		printf("File not found\n"); //the file couldn't be found
		return 0;
	}else{ //else, we can open the file
		*fp = fopen(token[1],"r"); //open the file for reading use only
		return 1; //return 1 if successfully completed
	}
}


int main(){
	
	return 0;
}
