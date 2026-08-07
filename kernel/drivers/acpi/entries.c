#include "drivers/acpi/entries.h"
#include "drivers/acpi/rsdp.h"
#include "mem/physical_mem.h"
#include <stdint.h>
#include <mem.h>
#include <terminal/printf.h>
#include <stddef.h>

struct Entries* entries;

uint32_t local_apic_address;

void entries_init() {
    entries = ((struct Table_Rev1_t*)entries_table)->Revision >= 2 ? (struct Entries*)((struct Table_Rev2_t*)entries_table)->XsdtAddress : (struct Entries*)(size_t)((struct Table_Rev1_t*)entries_table)->RsdtAddress;
}

void parse_entries() {
    int entries_count = (entries->header.Length - sizeof(entries->header)) / 4;

    for (int i = 0; i < entries_count; i++) {
        struct SDT_header *h = (struct SDT_header*)entries->entries[i];
        #ifdef DEBUG
            printf("0x%X\t", *(uint64_t*)h->Signature);
        #endif

        for (int y = 0; y < ACPITableSize; y++) {
            if ((*(uint32_t*)h->Signature) == ACPITable[y].key) {
                #ifdef DEBUG
                    printf("Has it own dedicated function, running...\n");
                #endif
                if (ACPITable[y].func) ACPITable[y].func(h);
                break;
            }
        }
        #ifdef DEBUG
            printf("\n");
        #endif
    }
}

char CPUs_ID[MAX_CPU_COUNT];
char CPUs_count = 0;
static void parse_apic(struct SDT_header* header) {
    struct APIC* apic = (struct APIC*)header;

    local_apic_address = apic->apic_addr;

    uint8_t* curl = (uint8_t*)&apic->flags + sizeof(int);
    uint8_t* end = (uint8_t*)apic + header->Length;

    CPUs_count = 0;

    while (curl < end) {
        struct APICEntry* ApicEntry = (struct APICEntry*)curl;

        switch (ApicEntry->entry_type) {
            case 0: // A physical processor
                CPUs_ID[CPUs_count++] = ((struct ProcessorLocalAPIC*)&ApicEntry->lenght + 1)->APIC_id;
                break;
            default:
                break;
        }

        curl += ApicEntry->lenght;
    }
    #ifdef DEBUG
        printf("APIC: Detected %d core/s", CPUs_count);
    #endif
}