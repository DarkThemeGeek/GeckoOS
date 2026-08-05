#include "boot/multiboot2.h"
#include "drivers/acpi.h"
#include "drivers/apic/lapic.h"
#include "drivers/mouse.h"
#include "terminal/printf.h"
#include <colors.h>
#include <commands.h>
#include <drivers/apic/ioapic.h>
#include <drivers/drives.h>
#include <drivers/keyboard.h>
#include <drivers/pci.h>
#include <drivers/serial.h>
#include <drivers/tables/idt.h>
#include <drivers/tables/irq.h>
#include <drivers/tables/PIT_timer.h>
#include <drivers/vga.h>
#include <layouts/kb_layouts.h>
#include <mem/paging.h>
#include <mem/physical_mem.h>
#include <net/arp.h>
#include <net/net.h>
#include <stdint.h>
#include <terminal/terminal.h>

void process_input(unsigned char *buffer) { run_command(buffer, TERM_COLOR); }

static void kmain();
void test_handler() { printf("INTERRUPT FIRED!\n"); }
__attribute__((section(".text.entry"))) void _entry(uint64_t magic,
                                                    uint64_t mbi_addr)
{

    serial_init();
    drives_init();

    vga_clear(TERM_COLOR);

    printc("GeckoOS Version 2.0\n", TERM_COLOR);
    printc("Booted via GRUB/Multiboot2.\n", TERM_COLOR);

    set_layout(LAYOUTS[0]);

    //======================Memory init(physical and virtual)=====================//
    /*
     * Bitmap goes at 0x500000 (512 bytes). Free region starts one page
     * later at 0x501000 so the allocator doesn't hand out its own bitmap
     * as the first allocation.
     */
    // acpi should read first what mem needs to be mapped than map it using the
    // long adress mapping tehnically
    initialize_memory_manager_from_mbi(mbi_addr);
    kalloc_init();

    if (!vmm_init()) {
        printc("vmm_init failed -- halting\n", VGA_COLOR_RED);
        for (;;)
            asm volatile("hlt");
    }
    outb(0x22, 0x70);
    outb(0x23, 0x01);

    //=====================Setting up interrupts===================//
    printf("Cpu has apic: %d \n", cpu_has_apic());
    int ret = acpi_init(magic, mbi_addr);
    if (ret != 0) {
        printf("intializzing apic failed: %d \n", ret);
    }

    if (cpu_has_apic()) {

        asm volatile("cli"); // cutting interrupts while we set em up
        printc("Mapping IDT... \n", VGA_COLOR_LIGHT_GREY);
        init_idt();
        printc("Installing IRQ... \n", VGA_COLOR_LIGHT_GREY);
        irq_install();
        printc("Setting up LAPIC.. \n", VGA_COLOR_LIGHT_GREY);
        ret = lapic_init();
        if (ret) {
            printf("Setting up LAPIC failed err %d", ret);
        }
        printc("Preapering IOAPIC.. \n", VGA_COLOR_LIGHT_GREY);
        ioapic_init();
        asm volatile("sti"); // repoening interrupts
    } else {
        // idk should have apic not my problem
    };
    //================= hardware init===================//
    printc("Enabling Timer ...\n", VGA_COLOR_LIGHT_GREY);
    lapic_timer_start();
    printc("Enabling hardware devices ...\n", VGA_COLOR_LIGHT_GREY);
    // basic stuff
    keyboard_install();
    mouse_init();
    terminal_init();
    // pci init
    enumerate_pci();
    pci_detect_nics();

    // network init//
    lapic_start_cores();
    net_init();
    arp_init();
    
     printc("Testing interruption...\n", VGA_COLOR_LIGHT_GREY);
    asm volatile("int $0x3");
    printc("Test completed!\n", VGA_COLOR_LIGHT_GREY);

    ;

    kmain();
}
static void kmain()
{

    get_kdrive(0);

    unsigned char mount_cmd[] = "fsmount";
    run_command(mount_cmd, TERM_COLOR);
    printc("\n", TERM_COLOR);
    while (1) {

        printc("gecko> ", PROMPT_COLOR);
        unsigned char buff[512];
        input(buff, 512, TERM_COLOR);
        process_input(buff);
    }
}