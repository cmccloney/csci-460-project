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
