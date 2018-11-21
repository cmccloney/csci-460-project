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

struct ffile {
	int start_cluster_parent;
	int start_cluster;
	int current_cluster;
	int current_cluster_index;
	short current_sector;
	short current_byte;
	unsigned int pos;
	unsigned char flags;
	unsigned char attribs;
	unsigned char mode;
	unsigned int size;
	char filename[256];
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

unsigned int first_sector(unsigned int cluster){
	unsigned int first_sector = ((cluster - 2)*fat_info.sectors_per_cluster) + fat_info.first_data_sector;
	return first_sector;
}

int seek(struct ffile *fp, unsigned int base, long offset){
	unsigned int index;
	long position = base + offset;
	unsigned int temp;
	if(position > fp->size){
		printf("error");
		return -1;
	}else if(position == fp->size){
		fp->size = fp->size + 1;
	}
	unsigned int b = 0x0ffffff8; //use to tell this is FAT32
	index = position / (fat_info.sectors_per_cluster*512); //grab the index of the cluster we want
	if(index != fp->current_cluster_index){ //if the current cluster is not the cluster we want, do this
		temp = index;
		if(index > fp->current_cluster_index){ //if it's larger,
			index = index - fp->current_cluster_index;
		}else{ //else if it's equal to or smaller
			fp->current_cluster = fp->start_cluster;
		}
		while(index > 0){ //iterate through each cluster
			temp = get_fat_entry(fp->current_cluster);
			if(temp & 0x0fffffff != b){
				fp->current_cluster = temp;
			}else{
				set_fat_entry(fp->current_cluster,temp); //create more space
				set_fat_entry(temp, b); //make new last cluster
				fp->current_cluster = temp; //same as above
			}
			index--; //next cluster
		}
	}
	fp->current_byte = position % (fat_info.sectors_per_cluster * 512); //calculate offset
	fp->pos = position;
	return 0; //might consider not returning any integer at all

}

int compare2(struct fat32_entry entry, unsigned char *name){
	int i;
	unsigned char file[16];
	int j = 0;
	for(i = 0; i < 8; i++){
		if(entry.filename[i] != ' '){
			file[j++] = entry.filename[i];
		}
	}
	file[j++] = '.';
	for(i = 0; i < 3; i++){ //second "half" of looking, after adding a dot to our file string
		if(entry.extension[i] != ' '){
			file[j++] = entry.extension[i];
		}
	}
	file[j++] = '\x00'; //necesarry at end of file
	i = 0; //reset
	if(i > 13){ //i > 13 reserved for longer names
		if(strncasecmp(name,file,13)){
			i = 0; //don't match
		}else{
			i = 1; //match
		}
	}else{
		if(file[i] != 0 || strncasecmp(name, file, i)){
			i = 0; //don't match
		}else{
			i = 1; //match
		}
	}
	return i;
}

unsigned int special_compare(struct ffile *fp, unsigned char *name){
	int i, j = 0;
	unsigned int name_length;
	struct fat32_entry entry;
	
	file_read((unsigned char*)&entry, sizeof(struct fat32_entry), fp); //still need to implement
	if(compare2(entry,name)){
		seek(fp, -sizeof(struct fat32_entry), fp->pos);
		return 1; //they match
	}else{
		return 0; //they don't match
	}
	//may need to add more here
	return 1;
}

int file_read(unsigned char *dest, int size, struct ffile *fp){
	unsigned int sector;
	while(size > 0){ //while there's still something to iterate through
		sector = first_sector(fp->current_cluster) + (fp->current_byte / 512);
		fetch(sector);
		*dest++ = fat_info.buffer[fp->current_byte % 512]; //hardcoded 512 because this is FAT32
		size--;
		if(fp->attribs & 0x10){
			if(seek(fp,0,fp->pos+1)){
				return -1;
			}
		}else{
			if(seek(fp,0,fp->pos +1)){ //similar thing
				return -1;
			}
		}
	}
	return 0; //otherwise return 0
}

int find_file(struct ffile *fp, unsigned char *name){
	unsigned int rc;
	seek(fp,0,0);
	while(1){
		rc = special_compare(fp,name);
		if(rc < 0) break;
		else if(rc == 1){ //file found
			return 0;
		}
	}
	return -1; 
}


unsigned char* traverse(unsigned char *filename, struct ffile *fp){
	struct fat32_entry entry;
	if(*filename == '/'){ //there's no path, is checking
		filename++;
		//smoething about '\x00' can be added
	}
	if(find_file(fp, filename)){
		printf("File not found\n");
		return NULL;
	}
	file_read((unsigned char*)&entry, sizeof(struct fat32_entry), fp); //need to implement
	while((*filename != '/') && (*filename != '\x00')){
		filename = filename + 1; //important, because we're returning filename
	}
	//also
	if(*filename == '/'){
		filename = filename + 1;
	}
	//now have info for next level in path, so
	fp->start_cluster_parent = fp->start_cluster;
	fp->start_cluster = ((unsigned int)(entry.cluster_hi & 0xffff) << 16) | (entry.first_cluster & 0xffff); //complex, see sources
	fp->attribs = entry.attributes;
	fp->current_cluster = fp->start_cluster;
	fp->current_cluster_index = 0; //just resetting these values based off next traversal (cluster)
	fp->current_sector = 0;
	fp->current_byte = 0;
	fp->pos = 0;
	if(entry.attributes & 0x10){
		fp->size = 0xffffffff;
	}else{
		fp->size = entry.file_size;
	}
	return filename;

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
