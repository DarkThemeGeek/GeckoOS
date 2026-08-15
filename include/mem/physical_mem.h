#pragma once
#include "boot/multiboot2.h"
#include <stdint.h>

#define AP_TRAMPOLINE_ADDR 0x8000
#define AP_TRAMPOLINE_SIZE 0x1000
//frees memory blocks
void free_blocks(const void *address, uint32_t num_blocks);
//sets (occupied or free) memory block at the current adress
void set_block(uint32_t bit);
//allocates blocks of memory for future use
void *allocate_blocks(uint32_t num_blocks);
//releases memory from the base adress and (does the oposite now for some reason //TODO: fix it)
void deinitialize_memory_region(uint64_t base_address, uint64_t size);


//TODO: replace mmap witht the one from boot 
//initializes the mem by using mmap from mbi
void initialize_memory_manager_from_mbi(uint64_t mbi_addr);

//finds the first free blocks for allocation sda
int32_t find_first_free_blocks(uint32_t num_blocks);
