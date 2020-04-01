#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Sizes in terms of bits */
#define ADDR_SIZE 32
#define PAGE_NUM_SIZE 8
#define OFFSET_SIZE 8

#define TLB_SIZE 16		// number of TLB entries
#define PT_SIZE 256		// number of page table entries
#define FRAME_SIZE 256  // number of bytes per frame
#define NUM_FRAMES 256  // number of frames in physical memory


/* Global data structures */
int8_t page_table[PT_SIZE];
int8_t physical_mem[FRAME_SIZE][NUM_FRAMES];

/* Structure for a single entry in the TLB */
typedef struct _TLB_ENTRY
{
    int page_num;
    int frame_num;    
} TLB_ENTRY;    

typedef struct _PT_ENTRY
{
    int frame_num;
    int valid;  //valid bit: 0 -> invalid; 1 -> valid
} PT_ENTRY;

PT_ENTRY page_table[PT_SIZE]; //Global page table

/* Display command line usage and exit */
void usage(char *command)
{
    fprintf(stderr, "usage: %s <filename>\n\n", command);
    fprintf(stderr, "filename - path to file containing logical "\
			"addresses to be read\n");
    exit(-1);
}

/* Search for a frame number at a given index in the page table */
int search_pt(int page_num)
{
    if(page_table[page_num].valid)        
        return page_table[page_num].frame_num;
    else
        return -1;
}

int main(int argc, char** argv)
{
    char *filename;
	char *mem_file = "BACKING_STORE.bin";
    FILE *fp;
    char str[5];  // Buffer to hold a single address read from file
    uint32_t address;  // Integer value of current address read from file     
    
    uint8_t page_num; // Page number parsed from logical address (bits 15 - 8)
    uint8_t offset;   // Offset within the frame indicated by page_num parsed from logical address (bits 7 - 0)
    
    TLB_ENTRY *tlb = (TLB_ENTRY*)malloc(sizeof(TLB_ENTRY) * TLB_SIZE);

    /* Parse filename from command line */
    if(argc == 1)
    {
        usage(argv[0]);
    }
    else if(argc > 2)
    {
        fprintf(stderr, "%s: Too many arguments\n", argv[0]);
        usage(argv[0]);
    }
    else
    {
        filename = argv[1];
    }
    
    /* Initialize page table elements to invalid*/
    for(int i = 0; i < PT_SIZE; i++)    
        page_table[i].valid = 0;
    
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
        if (address == 0) {    // ignore newline characters
            continue;
        }

        page_num = (address << (ADDR_SIZE - (PAGE_NUM_SIZE + OFFSET_SIZE))) >> (ADDR_SIZE - PAGE_NUM_SIZE);
        offset = (address << (ADDR_SIZE - OFFSET_SIZE)) >> (ADDR_SIZE - OFFSET_SIZE);

        printf("address: %x\npage_num: %x\noffset: %x\n", address, page_num, offset); // I used this for testing -- feel free to delete whenever

        /* TODO: check TLB for page_num. 

            TLB HIT: get the corresponding frame_num and access memory at that frame combined with the parsed offset.
           
            TLB MISS: Check page table.

                PT HIT: get the corresponding frame_num and access memory at that frame + offset. Update the TLB with page_num and frame_num.
                
                PT MISS: Handle page fault. Update PT and TLB.
        */            
        
    }
    
    /* Close file after all entries are read */
    fclose(fp);

    /* Free TLB memory */
    free(tlb);
    
    return(0);
}
