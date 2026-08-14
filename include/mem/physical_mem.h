#pragma once
#include "boot/multiboot2.h"
#include <stdint.h>

#define AP_TRAMPOLINE_ADDR 0x8000
#define AP_TRAMPOLINE_SIZE 0x1000

void free_blocks(const void *address, uint32_t num_blocks);

void set_block(uint32_t bit);

void *allocate_blocks(uint32_t num_blocks);

void deinitialize_memory_region(uint64_t base_address, uint64_t size);

void initialize_memory_manager_from_mbi(uint64_t mbi_addr);

int32_t find_first_free_blocks(uint32_t num_blocks);
