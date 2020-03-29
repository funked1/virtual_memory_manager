#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Sizes in terms of bits */
#define ADDR_SIZE 32
#define PAGE_NUM_SIZE 8
#define OFFSET_SIZE 8

/* Sizes in terms of entries */
#define TLB_SIZE 16    

/* Structure for a single entry in the TLB */
typedef struct _TLB_ENTRY
{
    int page_num;
    int frame_num;    
} TLB_ENTRY;    

int main(int argc, char** argv)
{
	char filename[] = "addresses.txt"; // File to read logical addresses from
	FILE *fp;
	char str[5];  // Buffer to hold a single address read from file
	uint32_t address;  // Integer value of current address read from file (32 bits)    
    
    uint8_t page_num; // Page number parsed from logical address (bits 15 - 8)
    uint8_t offset;   // Offset within the frame indicated by page_num parsed from logical address (bits 7 - 0)
    
    TLB_ENTRY *tlb = (TLB_ENTRY*)malloc(sizeof(TLB_ENTRY) * TLB_SIZE);
    
	/* Open file for reading */
	fp = fopen(filename, "r");
	if (fp == NULL) {
		perror("Error opening file");
		return(-1);
	}

	/* Read address file line by line */
	while (fgets(str, 6, fp) != NULL) {
		/* Convert address to int type */
		address = atoi(str);
		if (address == 0) {	// ignore newline characters
            continue;
		}

        page_num = (address << (ADDR_SIZE - (PAGE_NUM_SIZE + OFFSET_SIZE))) >> (ADDR_SIZE - PAGE_NUM_SIZE);
        offset = (address << (ADDR_SIZE - OFFSET_SIZE)) >> (ADDR_SIZE - OFFSET_SIZE);

        printf("address: %x\npage_num: %x\noffset: %x\n", address, page_num, offset); // I used this for testing -- feel free to delete whenever
	}

	/* Close file after all entries are read */
	fclose(fp);

	return(0);
}
