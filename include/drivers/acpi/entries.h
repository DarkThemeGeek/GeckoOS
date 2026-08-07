#pragma once

#include <stddef.h>
#include <stdint.h>

// This header and the other .c file have the RSDT and apic

#define APIC_TYPE_LOCAL_APIC            0
#define APIC_TYPE_IO_APIC               1
#define APIC_TYPE_INTERRUPT_OVERRIDE    2

#define MAX_CPU_COUNT   16

struct SDT_header {
    char Signature[4];
    uint32_t Length;
    uint8_t Revision;
    uint8_t Checksum;
    char OEMID[6];
    char OEMTableID[8];
    uint32_t OEMRevision;
    uint32_t CreatorID;
    uint32_t CreatorRevision;
} __attribute__ ((packed));

struct Entries {
    struct SDT_header header;
    uint32_t entries[];
} __attribute__ ((packed));

// APIC structs
struct APIC {
    struct SDT_header header;
    uint32_t apic_addr;
    uint32_t flags;
} __attribute__ ((packed));

struct APICEntry {
    char entry_type;
    char lenght;
} __attribute__ ((packed));

struct ProcessorLocalAPIC {
    char ACPI_procid;
    char APIC_id;
    uint32_t flags;
} __attribute__ ((packed));

struct IOAPIC {
    char IOAPIC_procid;
    char reserved;
    uint32_t IOAPIC_addr;
    uint32_t GSIB;
} __attribute__ ((packed));

struct IOAPIC_InterruptSourceOverride {
    char Bus_source;
    char IRQ_source;
    uint32_t GSI;
    uint16_t flags;
} __attribute__ ((packed));

struct IOAPIC_NonMaskableInterruptSource {
    char NMI_source;
    char reserved;
    uint16_t flags;
    uint32_t GSI;
} __attribute__ ((packed));

struct LocalAPIC_NonMaskableInterrupt {
    char ACPI_procid; // 0xFF means all processors
    uint16_t flags;
    char LINT; // Lint 0 OR 1
} __attribute__ ((packed));

struct LocalAPIC_AddressOverride {
    uint16_t reserved;
    uint64_t LocalAPIC_addr; // physical
} __attribute__ ((packed));

struct ProcLocal_x2apic {
    uint16_t reserved;
    uint32_t X2APIC_processors_local;
    uint32_t flags;
    uint32_t ACPI_id;
} __attribute__ ((packed));

static void parse_apic(struct SDT_header* apic);

struct ACPIItem {
    uint32_t key;
    void (*func)(struct SDT_header* entry);
};
static const struct ACPIItem ACPITable[22] = { // Every ACPI Signature is converted to an hexadecimal int
    {0x43495041, parse_apic}, // MADT
    // {0x54524542, NULL},
    // {0x50455043, NULL},
    // {0x54445344, NULL},
    // {0x54444345, NULL},
    // {0x4a4e4945, NULL},
    // {0x54535245, NULL},
    // {0x50434146, NULL}, // FADT
    // {0x53434146, NULL},
    // {0x54534548, NULL},
    // {0x5443534d, NULL},
    // {0x5453504d, NULL},
    // {0x784d4500, NULL},
    // {0x54544d50, NULL},
    // {0x54445350, NULL},
    // {0x46534152, NULL},
    // {0x54445352, NULL},
    // {0x54534253, NULL},
    // {0x54494c53, NULL}, // SLIT
    // {0x54415253, NULL}, // SRAT
    // {0x54445353, NULL}, // SSDT
    // {0x54445358, NULL}
};
static const size_t ACPITableSize = sizeof(ACPITable) / sizeof(struct ACPIItem);

void entries_init();
void parse_entries();

extern struct Entries* entries;