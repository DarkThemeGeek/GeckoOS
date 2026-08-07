#include "drivers/acpi/rsdt.h"
#include "drivers/acpi/rsdp.h"
#include "mem/physical_mem.h"
#include <stdint.h>
#include <mem.h>
#include <terminal/printf.h>
#include <stddef.h>

struct RSDT* rsdt;

uint32_t local_apic_address;


void rsdt_init() {
    rsdt = (struct RSDT*)get_rsdt();
}

_Bool check_rsdt() {
    for (int i = 0; i < (int)sizeof(rsdt->header.Signature); i++)
        if (rsdt->header.Signature[i] != RSDP_MAGIC[i]) return 0;

    uint8_t sum = 0;
    for (size_t i = 0; i < rsdt->header.Length; i++) sum += ((uint8_t *) rsdt)[i];
    return sum != 0;
}

void parse_rsdt_entries() {
    int entries = (rsdt->header.Length - sizeof(rsdt->header)) / 4;

    for (int i = 0; i < entries; i++) {
        struct SDT_header *h = (struct SDT_header*)rsdt->entries[i];
        
        for (int y = 0; y < ACPITableSize; y++) {
            if ((*(uint32_t*)h->Signature) == ACPITable[y].key) {
                if (ACPITable[y].func) ACPITable[y].func(h);
                break;
            }
        }
    }
}

char CPUs_ID[MAX_CPU_COUNT];
char CPUs_count = 0;
static void parse_apic(struct SDT_header* header) {
    struct APIC* apic = (struct APIC*)header;

    local_apic_address = apic->apic_addr;

    uint8_t* curl = (uint8_t*)&apic->flags + sizeof(int);
    uint8_t* end = (uint8_t*)apic + header->Length;

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
    printf("APIC: Detected %d cores\n", CPUs_count);
}