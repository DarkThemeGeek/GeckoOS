#include "drivers/mouse.h"
#include "drivers/tables/isr.h"
#include "drivers/tables/tss.h"
#include "mem.h"
#include <drivers/tables/idt.h>
#include <drivers/tables/irq.h>
#include <drivers/tables/timer.h>
#include <drivers/vga.h>
#include <drivers/keyboard.h>
#include <drivers/drives.h>
#include <drivers/serial.h>
#include <layouts/kb_layouts.h>
#include <terminal/terminal.h>
#include <commands.h>
#include <colors.h>
#include <stdint.h>
#include <drivers/pci.h>
#include <mem/physical_mem.h>
#include <mem/paging.h>
#include <net/net.h>
#include <net/arp.h>
#include <terminal/printf.h>
#include "fs/fs.h"
#include "ports.h"
#include <exe.h>
#include <boot/multiboot2.h>
#include <drivers/acpi.h>

void process_input(unsigned char *buffer) {
    run_command(buffer, TERM_COLOR);
}

void kmain();

extern struct multiboot2_tag_bootloader_name* bootloader_info;

// uint64_t global_table;

__attribute__((section(".text.entry")))
void _entry() {
    serial_init();
    kalloc_init();

    /*
     * Place the physical allocator bitmap immediately after BSS, page-aligned.
     * Using _bss_end from the linker script means this is always safe no matter
     * how large BSS grows — previously it was hardcoded to 0x500000 which BSS
     * was overrunning (console_history alone is ~2.5MB in BSS).
     */
    extern uint64_t _bss_end;
    uint64_t bitmap_start = ((uint64_t)&_bss_end + 0xFFF) & ~0xFFFULL;
    uint64_t free_start   = bitmap_start + 0x1000;
    uint64_t mem_end      = 0x1000000;

    initialize_memory_manager(bitmap_start, mem_end);
    initialize_memory_region(free_start, mem_end - free_start);

    vga_clear(TERM_COLOR);
    printc("GeckoOS Version 2.0\n", TERM_COLOR);
    #ifdef DEBUG
        printf("Booted via %s/Multiboot2.\n", bootloader_info->string);
    #else
        printc("Booted via GRUB/Multiboot2.\n", TERM_COLOR);
    #endif

    set_layout(LAYOUTS[0]);
    printc("Enabling IDT...\n", VGA_COLOR_LIGHT_GREY);
    init_idt();
    printc("Enabling IRQ...\n", VGA_COLOR_LIGHT_GREY);
    irq_install();
    printc("Enabling Timer (50Hz)...\n", VGA_COLOR_LIGHT_GREY);
    timer_install();
    keyboard_install();
    // mouse_init(); // This takes a long time in real machines, And i think this is useless unless you want to scroll
    terminal_init();
    timer_phase(50);

    printc("Enabling paging (Already activated from boot)...\n", VGA_COLOR_LIGHT_GREY);
    if (!vmm_init()) {
        printc("vmm_init failed -- halting\n", VGA_COLOR_RED);
        for (;;) asm volatile("hlt");
    }
    register_interrupt_handler(INT_PAGEFAULT, page_fault);

#ifdef DEBUG
    register_interrupt_handler(INT_INVINS, ud_exception_handler);
#endif

    acpi_init();

    drives_init();
    enumerate_pci();
    pci_detect_nics();

    net_init();
    arp_init();

    // global_table = (uint64_t)vmm_get_pml4();
    kmain();
}

void kmain() {
    // get_kdrive(0);
    STI();
    // vmm_set_pml4((page_table_t*)global_table);

    for (int i = 0; i < 4; i++) {
        printf("Trying drive %d", i);
        if (fsmount(i)) break;
    } if (!fs)
        printc("The drives 0 - 4 don't have any disk attached (Or it failed when mounting the FAT32 filesystem)\n\n", VGA_COLOR_RED);

    while (1) {
        printc("gecko> ", PROMPT_COLOR);
        unsigned char buff[512];
        input(buff, 512, TERM_COLOR);
        process_input(buff);
    }
}