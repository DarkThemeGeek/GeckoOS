#pragma once

#include <stdint.h>

struct RSDP_Rev1_t {
    char Signature[8];
    uint8_t Checksum;
    char OEMID[6];
    uint8_t Revision;
    uint32_t RsdtAddress; // Deprecated in version 2.0
} __attribute__ ((packed));

struct XSDP_Rev2_t {
    struct RSDP_Rev1_t header;
    uint32_t Length;
    uint64_t XsdtAddress;
    uint8_t ExtendedChecksum;
    uint8_t reserved[3];
} __attribute__ ((packed));

#define RSDP_MAGIC "RSD PTR "

void dumprsdp();
void rsdp_init();
_Bool check_rsdp();
struct SDT_header* get_rsdt();