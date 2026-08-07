#include "drivers/acpi/rsdt.h"
#include "mem/paging.h"
#include <drivers/acpi/rsdp.h>
#include <stddef.h>
#include <stdint.h>
#include <terminal/printf.h>
#include <boot/multiboot2.h>

struct RSDP_Rev1_t* rsdp;

extern struct multiboot2_tag_acpi* acpi_info_grub;
void rsdp_init() {
    rsdp = ((struct RSDP_Rev1_t*)acpi_info_grub->rsdp);
}

void dumprsdp() {
    printf("RSDP Signature: "); for (int i = 0; i < (int)sizeof(rsdp->Signature); i++) { printf("%c", rsdp->Signature[i]); }
    printf("\n");
    printf("RSDP OEMID: "); for (int i = 0; i < (int)sizeof(rsdp->OEMID); i++) { printf("%c", rsdp->OEMID[i]); }
    printf("\n");
    printf("RSDP Revision: %d\n", rsdp->Revision);
    printf("RSDP RSDT: 0x%p\n", rsdp->RsdtAddress);
}
struct SDT_header* get_rsdt() { return (struct SDT_header*)rsdp->RsdtAddress; }