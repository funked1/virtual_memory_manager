#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Sizes in terms of bits */
#define ADDR_SIZE 32
#define PAGE_NUM_SIZE 8
#define OFFSET_SIZE 8

#define TLB_SIZE 16 // TLB has 16 entries
#define PT_SIZE 256
#define NUM_FRAMES 256

int page_table[PT_SIZE]; //Global page table

/* Structure for a single entry in the TLB */
typedef struct _TLB_ENTRY
{
    int page_num;
    int frame_num;    
} TLB_ENTRY;    

int search_pt(int page_num)
{
    return page_table[page_num];
}

int main(int argc, char** argv)
{
    char filename[] = "addresses.txt"; // File to read logical addresses from
    FILE *fp;
    char str[5];  // Buffer to hold a single address read from file
    uint32_t address;  // Integer value of current address read from file (32 bits)    
    
    uint8_t page_num; // Page number parsed from logical address (bits 15 - 8)
    uint8_t offset;   // Offset within the frame indicated by page_num parsed from logical address (bits 7 - 0)
    
    TLB_ENTRY *tlb = (TLB_ENTRY*)malloc(sizeof(TLB_ENTRY) * TLB_SIZE);

    /* Initialize page table values to -1 (indicating empty)*/
    for(int i = 0; i < PT_SIZE; i++) 
        page_table[i] = -1;
    
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
