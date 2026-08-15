#pragma once
//VMM-Virtual memory manager
#include "drivers/tables/isr.h"
#include <stdint.h>
#include <stdbool.h>

#define PAGE_SIZE        4096ULL
#define PT_ENTRIES       512

#define PML4_INDEX(a)  (((uint64_t)(a) >> 39) & 0x1FF)
#define PDPT_INDEX(a)  (((uint64_t)(a) >> 30) & 0x1FF)
#define PD_INDEX(a)    (((uint64_t)(a) >> 21) & 0x1FF)
#define PT_INDEX(a)    (((uint64_t)(a) >> 12) & 0x1FF)

typedef enum {
    PTE_PRESENT        = (1ULL << 0),
    PTE_WRITABLE       = (1ULL << 1),
    PTE_USER           = (1ULL << 2),
    PTE_WRITE_THROUGH  = (1ULL << 3),
    PTE_CACHE_DISABLE  = (1ULL << 4),
    PTE_ACCESSED       = (1ULL << 5),
    PTE_DIRTY          = (1ULL << 6),
    PTE_HUGE           = (1ULL << 7),
    PTE_GLOBAL         = (1ULL << 8),
    PTE_NX             = (1ULL << 63),
    PTE_ADDR_MASK      = 0x000FFFFFFFFFF000ULL
} pte_flags_t;

typedef uint64_t pte_t;

typedef struct {
    pte_t entries[PT_ENTRIES];
} __attribute__((aligned(4096))) page_table_t;

static inline uint64_t pte_get_phys(pte_t e)   { return e & PTE_ADDR_MASK; }
static inline bool     pte_is_present(pte_t e) { return (e & PTE_PRESENT) != 0; }

//Initializing the memory
bool          vmm_init(void);
//maping an adress from it s physical position to a virtual one for the page table
bool          vmm_map(page_table_t *pml4, uint64_t phys, uint64_t virt, uint64_t flags);
//unmaping an adress from it s virtual one to the given page table
void          vmm_unmap(page_table_t *pml4, uint64_t virt);
//get the real adress from a virtual one in a page table
uint64_t      vmm_get_phys(page_table_t *pml4, uint64_t virt);
//get the current page table
page_table_t *vmm_get_pml4(void);
//set the current page table
bool          vmm_set_pml4(page_table_t *pml4);
//map a memory region to the current page table from the given adress and size of the region
uint64_t      mmio_map(uint64_t phys, uint64_t size);
//unmapping a region from the current page table
int mmio_unmap(uint64_t virt, uint64_t size);
//page fault handler function called by the isr(interupt) which handles the page faults 
void page_fault(registers_t* regs);
//allocs a page table
page_table_t *alloc_table(void);
// uint64_t mmio_map1(uint64_t phys, uint64_t virt, uint64_t flags, uint64_t size);