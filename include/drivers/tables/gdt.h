#pragma once

#include "drivers/tables/tss.h"
#include <stdint.h>

typedef struct Gdt_entry {
    uint16_t limitlow;
    uint16_t baselow;
    uint8_t basemiddle;
    uint8_t access;
    uint8_t granularity;
    uint8_t basehigh;
} __attribute__((packed)) Gdt_entry_t;

typedef struct Gdt {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) Gdt_t;

#define GDT_ENTRIES 6

extern Gdt_t gdt;
extern Gdt_entry_t gdt_entries[GDT_ENTRIES];

#define ACCESSED_BIT 1<<0
#define RW_BIT       1<<1
#define DC_BIT       1<<2
#define EXEC_BIT     1<<3
#define DESCR_BIT    1<<4
#define DPL_BIT      1<<5 | 1<<6
#define PRESENT_BIT  1<<7

void initgdt();