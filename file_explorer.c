#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

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

struct fs_attr{
 	unsigned short bytes_per_sector;
  	unsigned char sectors_per_cluster;
  	unsigned char num_fats;
  	unsigned short reserved_sector_count;
  	unsigned int fat_size;
  	char label[11];
}
