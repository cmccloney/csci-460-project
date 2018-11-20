#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int main(){
	printf("Main method\n");
	FILE * in = fopen("test.img","rb");
	unsigned int i, start_sector, length_sectors;

	fseek(in, 0x1BE, SEEK_SET); //go to start of partition table

	for(i = 0; i < 4; i++){ //read four etnries
				
	
	}
}
