#include "drivers/acpi/rsdt.h"
#include <drivers/acpi/rsdp.h>
#include <boot/multiboot2.h>
#include <stddef.h>
#include <stdint.h>
#include <terminal/printf.h>

extern struct multiboot2_tag_acpi* acpi_info_grub;
struct RSDP_Rev1_t* rsdp;

void rsdp_init() { rsdp = ((struct RSDP_Rev1_t*)acpi_info_grub->rsdp); }

void dumprsdp() {
    printf("RSDP Signature: "); for (int i = 0; i < (int)sizeof(rsdp->Signature); i++) { printf("%c", rsdp->Signature[i]); }
    printf("\n");
    printf("RSDP OEMID: "); for (int i = 0; i < (int)sizeof(rsdp->OEMID); i++) { printf("%c", rsdp->OEMID[i]); }
    printf("\n");
}
_Bool check_rsdp() { // True (1) if the RSDP is valid
    for (int i = 0; i < (int)sizeof(rsdp->Signature); i++) if (rsdp->Signature[i] != RSDP_MAGIC[i]) return 0;
    return 1;
}
struct SDT_header* get_rsdt() { return (struct SDT_header*)(size_t)rsdp->RsdtAddress; }