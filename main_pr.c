/**Dan Funke, Brian Rentsch
CSC345-0
Project 3 - Virtual Memory Manager - main_pr.c*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Sizes in terms of bits */
#define ADDR_SIZE 32
#define PAGE_NUM_SIZE 8
#define OFFSET_SIZE 8

#define TLB_SIZE 16     // number of TLB entries
#define PT_SIZE 256     // number of page table entries
#define FRAME_SIZE 256  // number of bytes per frame
#define NUM_FRAMES 128  // number of frames in physical memory (main_pr.c only has 128 physical frames)

/* Input and output filenames */
#define BACKING_STORE_FILENAME "BACKING_STORE.bin"
#define OUT1_FILENAME "out1.txt"
#define OUT2_FILENAME "out2.txt"
#define OUT3_FILENAME "out3.txt"
    
/* A single entry in the TLB */
typedef struct _TLB_ENTRY
{
    int page_num;
    int frame_num;
    int valid;  //valid bit: 0 -> invalid; 1 -> valid
} TLB_ENTRY;    

/* A single entry in the page table */
typedef struct _PT_ENTRY
{
    int frame_num;
    int valid;  //valid bit: 0 -> invalid; 1 -> valid
} PT_ENTRY;

TLB_ENTRY *tlb;
PT_ENTRY *pt;
int8_t physical_mem[NUM_FRAMES][FRAME_SIZE];

/* Display command line usage */
void usage(char *command)
{
    fprintf(stderr, "usage: %s <addresses_filename>\n\n", command);
    fprintf(stderr, "addresses_filename - path to file containing logical "\
            "addresses to be read\n");
}

/* Cleanup function to free memory before exiting */
void cleanup()
{
    free(tlb);
    free(pt);
}

/* Initialize the contents of physical memory frames to -1 */
void init_mem()
{
    for(int i = 0; i < NUM_FRAMES; i++)
    {
        for(int j = 0; j < FRAME_SIZE; j++)
            physical_mem[i][j] = -1;
    }
}

/* Initialize page table elements as invalid */
void init_pt()
{
    for(int i = 0; i < PT_SIZE; i++)    
        pt[i].valid = 0;              // valid bit of 0 = invalid
}

/* Search for a frame number at a given index in the page table */
int search_pt(uint8_t page_num)
{
    if(pt[page_num].valid)        
        return pt[page_num].frame_num;
    else
        return -1;
}

/* Update an entry in the page table and return next frame to be replaced */
int update_pt(uint8_t page_num, int frame_num)
{
    pt[page_num].valid = 1;   // Entry is valid
    pt[page_num].frame_num = frame_num;

    return ((frame_num + 1) % NUM_FRAMES);  // return number of next frame to be replaced according to FIFO
}

/* Print the contents of the page table */
void print_pt()
{
    printf("\tfn\tvalid\n");
    for(int i = 0; i < PT_SIZE; i++)    
        printf("[%d]:\t%d\t%d\n", i, pt[i].frame_num, pt[i].valid);     
}

/* Initialize TLB contents*/
void init_tlb()
{
    for(int i = 0; i < TLB_SIZE; i++)    
        tlb[i].valid = 0;    // initialize all TLB entries to invalid
}

/* Search TLB for a page number and return its frame number (if entry exists) */
int search_tlb(uint8_t page_num)
{
    int frame_num = -1; // assume TLB miss

    for(int i = 0; i < TLB_SIZE; i++)
    {
        if(tlb[i].page_num == page_num && tlb[i].valid)
        {
            frame_num = tlb[i].frame_num;
            break;
        }
    }

    return frame_num;
}

/* Update the TLB at the index indicated by TLB pointer using FIFO replacement */
int update_tlb(uint8_t page_num, int frame_num, int tlb_ptr)
{
    /* Update tlb entry */
    tlb[tlb_ptr].page_num = page_num;
    tlb[tlb_ptr].frame_num = frame_num;
    tlb[tlb_ptr].valid = 1;

    return ((tlb_ptr + 1) % TLB_SIZE);  //If all of the TLB entries have been filled, TLB pointer circles back to zero to enable FIFO replacement
}

/* Print the contents of the TLB */
void print_tlb()
{
    printf("\tpn\tfn\tvalid\n");
    for(int i = 0; i < TLB_SIZE; i++)    
        printf("[%d]:\t%d\t%d\t%d\n", i, tlb[i].page_num, tlb[i].frame_num, tlb[i].valid);     
}

/* Handle a page fault by reading data from the backing store and replacing a given frame with this new data */
int handle_page_fault(char* backing_store_fn, uint8_t page_num, int frame_ptr)
{
    int8_t buffer[256];

    /* Open backing store file */
    FILE *fp = fopen(backing_store_fn, "r");
    if(fp == NULL)
    {
        perror("Error opening memory file");
        return -1;
    }
    else
    {
        /* Get memory contents from bin file */
        fseek(fp, (page_num * FRAME_SIZE), SEEK_SET);
        fread(buffer, FRAME_SIZE, 1, fp);

        /* Fill frame with buffer contents */
        for(int i = 0; i < FRAME_SIZE; ++i)
        {
            physical_mem[frame_ptr][i] = buffer[i];
        }

        fclose(fp);
        return 0;
    }    
}

int main(int argc, char** argv)
{
    char *addresses_fn;  // Name of input file containing logical addresses
    char *backing_store_fn = BACKING_STORE_FILENAME;
    FILE *addr_fp, *out1, *out2, *out3;

    char str[7];            // Buffer to hold a single address read from file
    uint32_t page_address;  // Current logical address read from file     
    uint32_t frame_address; // Translated physical address
    
    uint8_t page_num; // Page number parsed from logical address (bits 15 - 8)
    uint8_t offset;   // Offset within the frame indicated by page_num parsed from logical address (bits 7 - 0)
    int frame_num;    // Frame number determined by either TLB or page table
    
    int frame_ptr = 0; // pointer to next frame to replace
    int tlb_ptr = 0;   // pointer to next TLB entry to replace

    int page_faults = 0;  // Total page faults 
    int tlb_hits = 0;     // Total TLB hits
    int total_references = 0;  // Total pages that have been referenced in the simulation
    
    int8_t byte_from_frame; // Byte read from physical memory
        
    /* Parse filename from command line */
    if(argc == 1)
    {
        usage(argv[0]);
        return -1;
    }
    else if(argc > 2)
    {
        usage(argv[0]);
        fprintf(stderr, "%s: Too many arguments\n", argv[0]);
        return -1;
    }
    else
    {
        addresses_fn = argv[1];  // Get path to file containing logical addresses from user
    }

    /* Allocate and initialize TLB and page table */
    tlb = (TLB_ENTRY*)malloc(sizeof(TLB_ENTRY) * TLB_SIZE);
    if(tlb == NULL)
    {
        perror("malloc");
        return -1;
    }
    pt = (PT_ENTRY*)malloc(sizeof(PT_ENTRY) * PT_SIZE);
    if(pt == NULL)
    {
        perror("malloc");
        return -1;
    }

    /* Initialize contents of TLB, page table, and physical memory */
    init_tlb();
    init_pt();
    init_mem(); 

    /* Open addresses file for reading */
    addr_fp = fopen(addresses_fn, "r");
    if(addr_fp == NULL)
    {
        perror(addresses_fn);
        cleanup();
        return -1;
    }
        
    /* Open output files for writing */
    out1 = fopen(OUT1_FILENAME, "w");
    if(out1 == NULL)
    {
        perror("Error opening file out1.txt");
        fclose(addr_fp);
        cleanup();
        return -1;
    }
    
    out2 = fopen(OUT2_FILENAME, "w");    
    if(out2 == NULL)
    {
        perror("Error opening file out2.txt");
        fclose(addr_fp);
        fclose(out1);
        cleanup();
        return -1;
    }
        
    out3 = fopen(OUT3_FILENAME, "w");
    if(out3 == NULL)
    {
        perror("Error opening file out3.txt");
        fclose(addr_fp);
        fclose(out1);
        fclose(out2);
        cleanup();
        return -1;
    }   

    /* Read address file line by line */
    while(fgets(str, 7, addr_fp) != NULL)
    {
        /* Convert address to int type */
        page_address = atoi(str);

        /* Parse address for page number and offset */
        page_num = (page_address << (ADDR_SIZE - (PAGE_NUM_SIZE + OFFSET_SIZE))) >> (ADDR_SIZE - PAGE_NUM_SIZE);
        offset = (page_address << (ADDR_SIZE - OFFSET_SIZE)) >> (ADDR_SIZE - OFFSET_SIZE);
        
        frame_num = search_tlb(page_num);  // search TLB for valid frame
        if(frame_num == -1)  
        {
            /* TLB MISS */
            frame_num = search_pt(page_num);  // search page table for valid frame
            if(frame_num == -1)  
            {
                /* PAGE TABLE MISS */
                page_faults++;
                handle_page_fault(backing_store_fn, page_num, frame_ptr);  // load data from backing store into next frame to update
                frame_num = frame_ptr;                                     // frame_ptr points to frame with newly loaded data

                /* Mark any entries in page table previously associated with the newly updated frame as invalid */
                for(int i = 0; i < PT_SIZE; i++) 
                {
                    if(pt[i].frame_num == frame_num)  // Invalidate any entries that were previously associated with frame_num
                        pt[i].valid = 0;
                }
                frame_ptr = update_pt(page_num, frame_num);                // update pt with newly loaded frame                
            }
            
            for(int i = 0; i < TLB_SIZE; i++) 
            {
                if(tlb[i].frame_num == frame_num)  // Invalidate any TLB entries that were previously associated with frame_num
                    tlb[i].valid = 0;
            }
            tlb_ptr = update_tlb(page_num, frame_num, tlb_ptr);    // update TLB with frame from page table
        }
        else
        {
            /* TLB HIT */
            tlb_hits++;
        }
        
        total_references++;
        
        frame_address = frame_num; // Get frame address by combining frame num and offset
        frame_address = (frame_address << OFFSET_SIZE) | offset;

        byte_from_frame = physical_mem[frame_num][offset];  // Get data from physical memory
        
        /* Generate output files */
        fprintf(out1, "%d\n", page_address);
        fprintf(out2, "%d\n", frame_address);
        fprintf(out3, "%d\n", byte_from_frame);
    }

    printf("Page faults = %d / %d, %2f\n", page_faults, total_references, ((double)page_faults / (double)total_references));
    printf("TLB hits = %d / %d, %2f\n", tlb_hits, total_references, ((double)tlb_hits / (double)total_references));
    
    /* Close files */
    fclose(addr_fp);
    fclose(out1);
    fclose(out2);
    fclose(out3);
    
    /* Free dynamically allocated memory */
    cleanup();
    
    return 0;
}
