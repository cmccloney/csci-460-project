#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

struct info {
	unsigned char sectors_per_cluster;
	unsigned int first_data_sector;
	unsigned int total_sectors;
	unsigned short reserved_sectors;
	unsigned int current_sector;
	unsigned char sector_flags;
	unsigned int root_dir_size;
	unsigned int fs;
	unsigned char buffer[512]; //512 bytes
};

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

struct bpb { //struct that defines boot sector, part I
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

struct info fat_info; //FAT info, global variable

int read_sector(unsigned char *data, unsigned int sector){ //read in a sector
	FILE *file;
	file = fopen("test.fat","r+b");
	fseek(file, sector*512,0); //512 bytes
	fread(data,1,512,file);
	fclose(file);
	return 0;
}

int write_sector(unsigned char *data, unsigned int sector){ //write to a sector
	FILE *file;
	file = fopen("test.fat","r+");
	fseek(file,sector*512,0);
	fwrite(data,1,512,file);
	fclose(file);
	return 0;
}

int fetch(unsigned int sector){ //fetch a given sector
	if(sector == fat_info.current_sector){
		return 0;
	}
	int new_sec = 0;
	//more can be added here
	new_sec |= read_sector(fat_info.buffer, sector); //read in sector
	if(!new_sec){
		fat_info.current_sector = sector;
	}
	return new_sec;
}

unsigned char checksum(unsigned char *pFcbName){ //look up for details
	int i;
	unsigned char sum = 0;
	for(i = 11; i; i--){
		sum = ((sum & 1) << 7) + (sum >> 1) + *pFcbName++;
	}
}

unsigned int set_fat_entry(unsigned int cluster, unsigned int val){
	unsigned int offset = cluster * 4;
	unsigned int c = fetch(fat_info.reserved_sectors + (offset/512));
	return c;
}

unsigned int get_fat_entry(unsigned int cluster){ //get fat entry for given cluster
	unsigned int offset = cluster * 4; //4 bytes, FAT32 style
	fetch(fat_info.reserved_sectors + (offset/512));
	return *((unsigned int *) &(fat_info.buffer[offset % 512]));
}

void bpbInit(struct bpb *BPB){
	BPB->jump_boot[0] = 0xEB;
	BPB->jump_boot[1] = 0x58;
	BPB->jump_boot[2] = 0x90;
	memcpy(BPB->oemname, "dosfs", 8);
	BPB->bytes_per_sector = (unsigned short) 512;
	BPB->sectors_per_cluster = 32;
	BPB->num_reserved_sectors = 32;
	BPB->num_fats = 2;
	BPB->root_max_entries = 0;
	BPB->total_sectors_16 = 0;

}

void init(){ //initialize the FAT32 structure
	struct bpb *BPB;
	unsigned int fat_size, root_dir_sectors, data_sectors, cluster_count; //more will be added here
	//init fat_info for running, and read in first sector, sector zero
	fat_info.current_sector = -1;
	fat_info.sector_flags = 0;
	fetch(0); //actually fetch sector 0
	fat_info.fs = open("test.fat", READ_ONLY);
	
	BPB = (struct bpb *) fat_info.buffer; //may not be necessary
	bpbInit(BPB); //initialize some BPB values
	
	if(BPB->bytes_per_sector != 512){ //check if BPB is structured right
		printf("%dError: wrong filesystem!\n",BPB->bytes_per_sector);
		return;
	}

	root_dir_sectors = ((BPB->root_max_entries * 32) + (BPB->bytes_per_sector - 1)) / 512; //check Microsoft's white paper for more info
										//on these formulas (pg. 13-)
	
	if(BPB->fat_size_16 != 0){
		fat_size = BPB->fat_size_16;
	}else{
		fat_size = BPB->fat32.fat_size;
	}
	
	if(BPB->total_sectors_16 != 0){
		fat_info.total_sectors = BPB->total_sectors_16;
	}else{
		fat_info.total_sectors = BPB->total_sectors_32;
	}

	data_sectors = fat_info.total_sectors - (BPB->num_reserved_sectors + (BPB->num_fats * fat_size) + root_dir_sectors);
	fat_info.sectors_per_cluster = BPB->sectors_per_cluster;
	cluster_count = data_sectors / fat_info.sectors_per_cluster;
	fat_info.reserved_sectors = BPB->num_reserved_sectors;
	fat_info.first_data_sector = BPB->num_reserved_sectors + (BPB->num_fats * fat_size) + root_dir_sectors;
	
	if(cluster_count < 4085){
		printf("Error: FAT12\n");
		return;
	}else if(cluster_count < 65525){
		printf("Error: FAT16\n");
		return;
	}else{
		printf("Volume is FAT32\n");
	}
	
	fat_info.root_dir_size = 0xffffffff;



}

int main(int argc, char **argv) {
	
	init();
	return 0;   
}
