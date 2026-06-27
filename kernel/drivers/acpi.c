#include "drivers/acpi.h"
#include "boot/multiboot2.h"
#include "drivers/vga.h"
#include "mem/paging.h"
#include "string.h"
#include "terminal/terminal.h"
#include <stdint.h>
#include <terminal/printf.h>
#define NULL (void *)0

// Iterate trough multi boot info structure tags and search for rsdp v1 or v2
void *find_rsdp(uint64_t magic, uint64_t mbi_addr)
{

    uint8_t *mbi        = (uint8_t *)mbi_addr;
    uint32_t total_size = *(uint32_t *)mbi;

    multiboot2_tag_t *tag = (multiboot2_tag_t *)(mbi + 8);
    uintptr_t mbi_end     = (uintptr_t)mbi + total_size;

    while ((uintptr_t)tag < mbi_end && tag->type != MULTIBOOT2_TAG_TYPE_END) {
        // printf("tag at %p: type=%d size=%d\n", tag, tag->type, tag->size);

        // ONLY process ACPI tags - skip everything else
        if (tag->type == MULTIBOOT2_TAG_TYPE_ACPI_NEW) {
            multiboot2_tag_acpi_t *acpi = (multiboot2_tag_acpi_t *)tag;
            // printf("Found ACPI v2 RSDP at %p\n", acpi->rsdp);
            return acpi->rsdp;
        } else if (tag->type == MULTIBOOT2_TAG_TYPE_ACPI_OLD) {
            multiboot2_tag_acpi_t *acpi = (multiboot2_tag_acpi_t *)tag;
            // printf("Found ACPI v1 RSDP at %p\n", acpi->rsdp);
            return acpi->rsdp;
        } else {
            // Skip all other tag types
           
        }

        tag = (multiboot2_tag_t *)((uintptr_t)tag + ((tag->size + 7) & ~7));
    }

    printf("No ACPI tag found!\n");
    return NULL;
}

uintptr_t acpi_lapic_base  = 0;
uintptr_t acpi_ioapic_base = 0;
// Find MADT via XSDT (ACPI 2.0+)
static struct acpi_madt *find_madt_via_xsdt(uint64_t xsdt_phys)
{
    // Map just the header first to get length
    struct acpi_header *header =
        (struct acpi_header *)mmio_map(xsdt_phys, sizeof(struct acpi_header));
    if (!header || memcmp(header->signature, "XSDT", 4) != 0) {
        return NULL;
    }

    uint32_t length = header->length;
    mmio_unmap((uintptr_t)header, sizeof(struct acpi_header));

    // Map the entire XSDT
    struct acpi_header *xsdt =
        (struct acpi_header *)mmio_map(xsdt_phys, length);
    if (!xsdt)
        return NULL;

    // Number of entries (each is 8 bytes for XSDT)
    int count = (xsdt->length - sizeof(struct acpi_header)) / 8;
    uint64_t *entries =
        (uint64_t *)((uint8_t *)xsdt + sizeof(struct acpi_header));

    struct acpi_madt *madt = NULL;

    for (int i = 0; i < count; i++) {
        // Map each table header to check signature
        struct acpi_header *hdr = (struct acpi_header *)mmio_map(
            entries[i], sizeof(struct acpi_header));
        if (!hdr)
            continue;

        if (memcmp(hdr->signature, "APIC", 4) == 0) {
            // Found MADT - map the entire table
            uint32_t madt_len = hdr->length;
            mmio_unmap((uintptr_t)hdr, sizeof(struct acpi_header));

            madt = (struct acpi_madt *)mmio_map(entries[i], madt_len);
            break;
        }

        mmio_unmap((uintptr_t)hdr, sizeof(struct acpi_header));
    }

    mmio_unmap((uintptr_t)xsdt, length);
    return madt;
}

// Find MADT via RSDT (ACPI 1.0)
static struct acpi_madt *find_madt_via_rsdt(uint32_t rsdt_phys)
{
    // Map just the header first
    struct acpi_header *header =
        (struct acpi_header *)mmio_map(rsdt_phys, sizeof(struct acpi_header));
    if (!header || memcmp(header->signature, "RSDT", 4) != 0) {
        return NULL;
    }

    uint32_t length = header->length;
    mmio_unmap((uintptr_t)header, sizeof(struct acpi_header));

    // Map the entire RSDT
    struct acpi_header *rsdt =
        (struct acpi_header *)mmio_map(rsdt_phys, length);
    if (!rsdt)
        return NULL;

    // Number of entries (each is 4 bytes for RSDT)
    int count = (rsdt->length - sizeof(struct acpi_header)) / 4;
    uint32_t *entries =
        (uint32_t *)((uint8_t *)rsdt + sizeof(struct acpi_header));

    struct acpi_madt *madt = NULL;

    for (int i = 0; i < count; i++) {
        // Map each table header to check signature
        struct acpi_header *hdr = (struct acpi_header *)mmio_map(
            entries[i], sizeof(struct acpi_header));
        if (!hdr)
            continue;

        if (memcmp(hdr->signature, "APIC", 4) == 0) {
            // Found MADT - map the entire table
            uint32_t madt_len = hdr->length;
            mmio_unmap((uintptr_t)hdr, sizeof(struct acpi_header));

            madt = (struct acpi_madt *)mmio_map(entries[i], madt_len);
            break;
        }

        mmio_unmap((uintptr_t)hdr, sizeof(struct acpi_header));
    }

    mmio_unmap((uintptr_t)rsdt, length);
    return madt;
}

// Parse MADT entries
static int parse_madt_entries(struct acpi_madt *madt)
{
    if (!madt)
        return 0;

    // Map LAPIC base
    acpi_lapic_base =madt->lapic_addr;
    printf("LAPIC is at %p\n", acpi_lapic_base);

    // Parse entries after the MADT header
    uint8_t *ptr = (uint8_t *)madt + sizeof(struct acpi_madt);
    uint8_t *end = (uint8_t *)madt + madt->header.length;

    int found_ioapic = 0;

    while (ptr < end) {
        struct madt_entry_header *h = (struct madt_entry_header *)ptr;

        // Sanity check
        if (h->length == 0 || ptr + h->length > end) {
            // printf("Invalid MADT entry at %p (length %d)\n", ptr, h->length);
            break;
        }

        switch (h->type) {
        case 0: // LAPIC
                //   printf("Found LAPIC at entry %d\n", found_ioapic);
            break;

        case 1: { // IOAPIC
            struct madt_ioapic *io = (struct madt_ioapic *)ptr;
            acpi_ioapic_base       = io->addr;//mmio_map(io->addr, PAGE_SIZE);
            printf("IOAPIC found: addr=%p, gsi_base=%u\n", acpi_ioapic_base,
                   io->gsi_base);
            found_ioapic = 1;
            break;
        }

        case 2: { // ISO (Interrupt Source Override)
            struct madt_iso *iso = (struct madt_iso *)ptr;
            // printf("ISO: bus=%u, irq=%u, gsi=%u, flags=0x%x\n",
            // iso->bus,iso->irq, iso->gsi, iso->flags);
            break;
        }
        case 3: {
            break;
        }
        case 4: {
            break;
        }
        case 5: {
            break;
        }
        default:
            printf("Unknown MADT entry type %d\n", h->type);
            break;
        }

        ptr += h->length;
    }

    return found_ioapic;
}

// Function that does the init for acpi and other stuff that needs parsing from
// mbi_addr including lapic and ioapic
int acpi_init(uint64_t magic, uint64_t mbi_addr)
{
    // printf("=== ACPI INIT START ===\n");
    // printf("magic=0x%lx, mbi_addr=0x%lx\n", magic, mbi_addr);

    struct acpi_rsdp_v1 *rsdp =
        (struct acpi_rsdp_v1 *)find_rsdp(magic, mbi_addr);

    if (!rsdp) {
        printf("FAIL: No RSDP found\n");
        return -1;
    }
    // debuging forgot to enable maping hate this shit
    //  printf("RSDP found at %p\n", rsdp);
    //  printf("RSDP revision: %d\n", rsdp->revision);
    //  printf("RSDP OEM ID: %.6s\n", rsdp->oem_id);
    //  printf("RSDT addr: 0x%x\n", rsdp->rsdt_addr);
    // check if revision exists we move to v2
    if (rsdp->revision >= 2) {
        struct acpi_rsdp_v2 *rsdp_v2 = (struct acpi_rsdp_v2 *)rsdp;
        // printf("XSDT addr: 0x%lx\n", rsdp_v2->xsdt_addr);
        // printf("RSDP length: %d\n", rsdp_v2->length);
    }

    struct acpi_madt *madt = NULL;

    // Try XSDT first v2 if revision exists
    if (rsdp->revision >= 2) {
        struct acpi_rsdp_v2 *rsdp_v2 = (struct acpi_rsdp_v2 *)rsdp;
        if (rsdp_v2->xsdt_addr != 0) {
            //   printf("\nTrying XSDT at 0x%lx\n", rsdp_v2->xsdt_addr);

            // Map XSDT header
            struct acpi_header *xsdt_header = (struct acpi_header *)mmio_map(
                rsdp_v2->xsdt_addr, sizeof(struct acpi_header));
            if (!xsdt_header) {
                printf("FAIL: Could not map XSDT header\n");
            } else {
                // printf("XSDT signature: %.4s\n",
                // xsdt_header->signature);
                // printf("XSDT length: %u\n", xsdt_header->length);

                if (memcmp(xsdt_header->signature, "XSDT", 4) == 0) {
                    // Map full XSDT
                    struct acpi_header *xsdt = (struct acpi_header *)mmio_map(
                        rsdp_v2->xsdt_addr, xsdt_header->length);
                    if (xsdt) {
                        int count =
                            (xsdt->length - sizeof(struct acpi_header)) / 8;
                        // printf("XSDT has %d entries\n", count);

                        uint64_t *entries =
                            (uint64_t *)((uint8_t *)xsdt +
                                         sizeof(struct acpi_header));
                        for (int i = 0; i < count; i++) {
                            // printf("  Entry %d: 0x%lx\n", i,
                            // entries[i]);

                            // Map just the header of each table
                            struct acpi_header *hdr =
                                (struct acpi_header *)mmio_map(
                                    entries[i], sizeof(struct acpi_header));
                            if (!hdr) {
                                printf("FAIL: Could notmap header\n");
                                continue;
                            }
                            // printf("Signature: %.4s\n",hdr->signature);
                            if (memcmp(hdr->signature, "APIC", 4) == 0) {
                                // printf("    *** FOUND MADT!**\n");
                                mmio_unmap((uintptr_t)hdr,
                                           sizeof(struct acpi_header));
                                // Map full MADT
                                madt = (struct acpi_madt *)mmio_map(
                                    entries[i], hdr->length);
                                // printf("    MADT mapped at
                                // %p,length=%u\n",madt, hdr->length);
                                break;
                            }

                            // Headers are only mapped not unmapped memory
                            // deallocation is for losers
                        }
                    }
                } else {
                    printf("Invalid XSDT signature\n");
                }
            }
        }
    }

    // Try RSDT fallback
    if (!madt && rsdp->rsdt_addr != 0) {
        //  printf("\nTrying RSDT at 0x%x\n", rsdp->rsdt_addr);

        struct acpi_header *rsdt_header = (struct acpi_header *)mmio_map(
            rsdp->rsdt_addr, sizeof(struct acpi_header));
        // printf("\n %p \n", rsdt_header);
        if (rsdt_header) {
            //   printf("RSDT signature: %.4s\n", rsdt_header->signature);
            // printf("RSDT length: %u\n", rsdt_header->length);

            if (memcmp(rsdt_header->signature, "RSDT", 4) == 0) {
                struct acpi_header *rsdt = (struct acpi_header *)mmio_map(
                    rsdp->rsdt_addr, rsdt_header->length);
                if (rsdt) {
                    int count = (rsdt->length - sizeof(struct acpi_header)) / 4;
                    // printf("RSDT has %d entries\n", count);
                    uint32_t *entries =
                        (uint32_t *)((uint8_t *)rsdt +
                                     sizeof(struct acpi_header));

                    for (int i = 0; i < count; i++) {
                        // printf("  %d: 0x%x\n", i, entries[i]);

                        struct acpi_header *hdr =
                            (struct acpi_header *)mmio_map(
                                entries[i], sizeof(struct acpi_header));
                        if (!hdr)
                            continue;
                        // printf("    Signature: %.4s\n",hdr->signature);
                        if (memcmp(hdr->signature, "APIC", 4) == 0) {
                            // printf("    *** FOUND MADT! ***\n");
                            madt = (struct acpi_madt *)mmio_map(entries[i],
                                                                hdr->length);
                            if (!madt) {
                                // printf("MADT mapped at/%p\n", madt);
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    if (!madt) {
        printf("\nFAIL: MADT not found in any table\n");
        printf("This means either:\n");
        printf("  1. ACPI tables are corrupted\n");
        printf("  2. Memory mapping is failing\n");
        printf("  3. Your MMIO mapping doesn't work for physical addresses > "
               "4GB\n");
        printf("  4. find_rsdp() returned a pointer to unmapped memory\n");
        return -2;
    }

    printc("MADT found successfully!\n", VGA_COLOR_LIGHT_GREY);
    // printf("LAPIC address: 0x%x\n", madt->lapic_addr);

    parse_madt_entries(madt);
    // printf("LAPIC mapped at %p\n", acpi_lapic_base);

    return 0;
}