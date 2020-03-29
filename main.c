#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define TLB_SIZE 16

typedef struct _TLB_ENTRY
{
    int page_num;
    int frame_num;    
} TLB_ENTRY;    

int main(int argc, char** argv)
{
	char filename[] = "addresses.txt";
	FILE *fp;
	char str[5];
	int address;

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
	}

	/* Close file after all entries are read */
	fclose(fp);

	return(0);
}
