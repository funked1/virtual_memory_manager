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
#define NUM_FRAMES 256  // number of frames in physical memory

/* output file names */
const char file_names[3][10] = {
    "out1.txt",
    "out2.txt",
    "out3.txt"
};
    
/* Structure for a single entry in the TLB */
typedef struct _TLB_ENTRY
{
    int page_num;
    int frame_num;
    int valid;  //valid bit: 0 -> invalid; 1 -> valid
} TLB_ENTRY;    

/* Structure for a single entry in the page table */
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

/* Initialize page table elements as invalid*/
void init_pt()
{
    for (int i = 0; i < PT_SIZE; i++)    
        pt[i].valid = 0;              // initialize all pt entries to invalid
}

/* Search for a frame number at a given index in the page table */
int search_pt(uint8_t page_num)
{
    if(pt[page_num].valid)        
        return pt[page_num].frame_num;
    else
        return -1;
}

/* Update the tlb at the index indicated by tlb pointer using FIFO replacement */
int update_pt(uint8_t page_num, int frame_num)
{
    /* Update page table entry */
    pt[page_num].valid = 1;
    pt[page_num].frame_num = frame_num;

    return ((frame_num + 1) % NUM_FRAMES);  // return number of next frame to be replaced according to FIFO
}

/* Initialize all tlb elements as empty*/
void init_tlb()
{
    for (int i = 0; i < TLB_SIZE; i++)    
        tlb[i].valid = 0;    // initialize all tlb entries to invalid
}

/* Search TLB for a page number, get index of TLB entry with that page number */
int search_tlb(uint8_t page_num)
{
    int frame_num = -1; // assume tlb miss

    for (int i = 0; i < TLB_SIZE; i++)
    {
        if (tlb[i].page_num == page_num && tlb[i].valid)
        {
            frame_num = tlb[i].frame_num;
            break;
        }
    }

    return frame_num;
}

/* Update the tlb at the index indicated by tlb pointer using FIFO replacement */
int update_tlb(uint8_t page_num, int frame_num, int tlb_ptr)
{
    /* Update tlb entry */
    tlb[tlb_ptr].page_num = page_num;
    tlb[tlb_ptr].frame_num = frame_num;
    tlb[tlb_ptr].valid = 1;

    return ((tlb_ptr + 1) % TLB_SIZE);  //If all of the TLB entries have been filled, TLB pointer circles back to zero to enable FIFO replacement
}

int handle_page_fault(char* backing_store_fn, uint8_t page_num, int frame_ptr)
{
    int8_t buffer[256];
    FILE *fp = fopen(backing_store_fn, "r");
    if (fp == NULL)
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
        for (int i = 0; i < FRAME_SIZE; ++i)
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
    char *backing_store_fn = "BACKING_STORE.bin";
    FILE *addr_fp, *out1, *out2, *out3;

    char str[5];       // Buffer to hold a single address read from file
    uint32_t page_address;  // Current logical address read from file     
    uint32_t frame_address; // Translated physical address
    
    uint8_t page_num; // Page number parsed from logical address (bits 15 - 8)
    uint8_t offset;   // Offset within the frame indicated by page_num parsed from logical address (bits 7 - 0)
    int frame_num;    // Frame number determined by either TLB or page table
    
    int frame_ptr = 0; // pointer to next frame to replace
    int tlb_ptr = 0; // pointer to next tlb entry to replace

    int page_faults = 0;
    int tlb_hits = 0;
    
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
        addresses_fn = argv[1];
    }

    /* Allocate and initialize TLB and page table */
    tlb = (TLB_ENTRY*)malloc(sizeof(TLB_ENTRY) * TLB_SIZE);
    pt = (PT_ENTRY*)malloc(sizeof(PT_ENTRY) * PT_SIZE);
        
    init_tlb();
    init_pt();
    
    /* Open addresses file for reading */
    addr_fp = fopen(addresses_fn, "r");
    if (addr_fp == NULL)
    {
        perror(addresses_fn);
        return -1;
    }
        
    /* Open output files for writing */
    out1 = fopen("out1.txt", "w");
    if (out1 == NULL)
    {
        perror("Error opening file out1.txt");
    }
    
    out2 = fopen("out2.txt", "w");    
    if (out2 == NULL)
    {
        perror("Error opening file out2.txt");
        fclose(addr_fp);
        fclose(out1);
        return -1;
    }
        
    out3 = fopen("out3.txt", "w");
    if (out3 == NULL)
    {
        perror("Error opening file out3.txt");
        fclose(addr_fp);
        fclose(out1);
        fclose(out2);
        return -1;
    }
    
    /* Read address file line by line */
    while (fgets(str, 6, addr_fp) != NULL)
    {
        /* Convert address to int type */
        page_address = atoi(str);
        if (page_address == 0) // ignore newline characters
        {    
            continue;
        }

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
                frame_ptr = update_pt(page_num, frame_num);                // update pt with newly loaded frame
            }
            
            tlb_ptr = update_tlb(page_num, frame_num, tlb_ptr);    // update TLB with frame from page table
        }
        else
        {
            /* TLB HIT */
            tlb_hits++;
        }
        
        frame_address = frame_num; // Get frame address by combining frame num and offset
        frame_address = (frame_address << OFFSET_SIZE) | offset;

        byte_from_frame = physical_mem[frame_num][offset];  // Get data from physical memory by
        
        /* Generate output files */
        fprintf(out1, "%d\n", page_address);
        fprintf(out2, "%d\n", frame_address);
        fprintf(out3, "%d\n", byte_from_frame);        

        //printf("page_address: %x\npage_num: %x\noffset: %x\n", page_address, page_num, offset); // I used this for testing -- feel free to delete whenever        
    }
        
    /* Close files */
    fclose(addr_fp);
    fclose(out1);
    fclose(out2);
    fclose(out3);

    /* Free dynamically allocated memory */
    free(tlb);
    free(pt);
    
    return 0;
}
