#ifndef PAGING_H
#define PAGING_H

#include "utils.h"

extern void loadPageDirectory(unsigned int* dir);
extern void enablePaging();

#define PAGE_PRESENT 1
#define PAGE_RW      2
#define PAGE_USER    4
#define PAGE_DIRTY   16

typedef struct {
        u32int present    : 1;   // Page present in memory
        u32int rw         : 1;   // Read-only if clear, readwrite if set
        u32int user       : 1;   // Supervisor level only if clear
        u32int accessed   : 1;   // Has the page been accessed since last refresh?
        u32int dirty      : 1;   // Has the page been written to since last refresh?
        u32int unused     : 7;   // Amalgamation of unused and reserved bits
        u32int page       : 20;  // Page address (shifted right 12 bits)
} page_table_entry_t;

typedef union {
    page_table_entry_t page_entry;
    u32int value;
} PTE; // PTE = Page Table Entry 

typedef PTE * PageTable;

typedef struct {
        u32int present    : 1;   // Page table is present in memory
        u32int rw         : 1;   // Read-only if clear, readwrite if set
        u32int user       : 1;   // Supervisor level only if clear
        u32int reserved   : 9;   // Some useful bits not we dont use them for now
        u32int page_table : 20;  // Page table address
} page_dir_entry_t;

typedef union {
    page_dir_entry_t dir_entry;
    u32int value;
} PDE;

typedef PDE * PageDirectory;

void initialise_paging();
PageTable alloc_page_entry(u32int address, int is_writeable, int is_kernel );

#endif
