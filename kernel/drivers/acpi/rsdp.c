#include "mem/paging.h"
#include <drivers/acpi/rsdp.h>
#include <stddef.h>
#include <stdint.h>
#include <terminal/printf.h>
#include <boot/multiboot2.h>

void* entries_table;

extern struct multiboot2_tag_acpi* acpi_info_grub;
void table_init() {
    entries_table = ((struct RSDP_Rev1_t*)acpi_info_grub->rsdp);
}

void dumptable() {
    const struct Table_Rev2_t* table = (const struct Table_Rev2_t*)entries_table;

    printf("Table Signature: "); for (int i = 0; i < (int)sizeof(table->header.Signature); i++) { printf("%c", table->header.Signature[i]); }
    printf("\n");
    printf("Table OEMID: "); for (int i = 0; i < (int)sizeof(table->header.OEMID); i++) { printf("%c", table->header.OEMID[i]); }
    printf("\n");
    printf("Table Revision: %d\n", table->header.Revision);
    if (table->header.Revision <= 1) printf("Table RSDT: 0x%p\n", table->header.RsdtAddress);

    if (table->header.Revision >= 2) goto Rev2;
    return;
Rev2:
    printf("Table XSDT: 0x%p\n", table->XsdtAddress);
}