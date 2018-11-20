#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

struct boot_32 { //struct that defines boot sector, part II, relevant for FAT32
	unsigned int fat_size; //starts at 36
	unsigned short ext_flags; //2
	unsigned short fs_version;
	unsigned int root_cluster;
	unsigned short fs_info; //sector
	unsigned short backup_sector;
	unsigned char reserved[12];
	unsigned char drive_number;
	unsigned char reserved2;
	unsigned char boot_sig;
	unsigned int volume_id;
	char label[11];
	char type[8];
}__attribute__((__packed__));

struct boot_sector { //struct that defines boot sector, part I
    	unsigned char jump_boot[3]; //starts at 0
    	char oemname[8]; 
    	unsigned short bytes_per_sector;
    	unsigned char sectors_per_cluster;
    	unsigned short num_reserved_sectors; 
    	unsigned char num_fats; 
    	unsigned short root_max_entries;
    	unsigned short total_sectors_16; //FAT16 info included only for necessity
    	unsigned char media_info; 
    	unsigned short fat_size_16;
    	unsigned short num_heads;
    	unsigned int hidden_sectors;
    	unsigned int total_sectors_32;
    	struct boot_32 fat32;
}__attribute__((__packed__));

struct fat32_entry{
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
}__attribute__((__packed__));

struct fat32_entry_long {
	unsigned char sequence_number;
	unsigned short name1[5];
	unsigned char attributes;
	unsigned char type;
	unsigned char checksum;
	unsigned short name2[6];
	unsigned short first_cluster; //defined as 0x0000 always
	unsigned short name3[2];
}__attribute__((__packed__));

//#define SEQ_START 0x40
//#define SEQ_DELETED 0x80
//#define SEQ_MASK 0x3f

#define READ_ONLY 0x01
#define HIDDEN 0x02
#define SYSTEM 0x04
#define VOLUME_ID 0x08
#define DIRECTORY 0x10
#define ARCHIVE 0x20

#define READ 0x01 //these included, may not be used
#define WRITE 0x02
#define APPEND 0x04
#define OVERWRITE 0x08

int main() {
	return 0;   
}
