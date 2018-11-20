#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct { //struct that defines partition table
    unsigned char first_byte;
    unsigned char start_chs[3];
    unsigned char partition_type;
    unsigned char end_chs[3];
    unsigned long start_sector;
    unsigned long length_sectors;
} __attribute((packed)) PartitionTable;

int main() {
    FILE * in = fopen("test.img", "rb"); //read in image file
    int i;
    PartitionTable pt[4]; //partition table is array
    
    fseek(in, 0x1BE, SEEK_SET); // go to partition table start
    fread(pt, sizeof(PartitionTable), 4, in); // read all four entries
    
    for(i=0; i<4; i++) { //read all four entries
        printf("Partition %d, type %02Xn", i, pt[i].partition_type);
        printf("  Start sector %08X, %d sectors longn", 
                pt[i].start_sector, pt[i].length_sectors);
    }
    
    fclose(in); //close file
    return 0;
}
