#include "drivers/apic/ioapic.h"
#include "drivers/apic/lapic.h"
#include "drivers/vga.h"
#include "gk/gk.h"
#include <drivers/tables/idt.h>
#include <drivers/tables/isr.h>
#include <ports.h>
#include <stdint.h>
#include <terminal/printf.h>
#include <terminal/terminal.h>
extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();
extern void irq_common_stub();
#define MAX_IRQS 223

extern void (*irq_stub_table[MAX_IRQS])(void);
//TODO:remeber this are numbered 32->0 and so on wen dealing with vectors
//!so the index means the irq not the idt number note for me
isr_t irq_routines_table[MAX_IRQS];

void irq_install_handler(int irq, isr_t handler, int flags)
{

    irq_routines_table[irq] = handler;
    ioapic_redirect_irq(irq, irq + 32, flags);
    
}   

void irq_uninstall_handler(int irq) { irq_routines_table[irq] = 0; }

/* void irq_remap(void)
{
    // outb(0x20, 0x11);
    // outb(0xA0, 0x11);
    // outb(0x21, 0x20);
    // outb(0xA1, 0x28);
    // outb(0x21, 0x04);
    // outb(0xA1, 0x02);
    // outb(0x21, 0x01);
    // outb(0xA1, 0x01);
    // outb(0x21, 0x0);
    // outb(0xA1, 0x0);
} */

void irq_install()
{
    for (int i = 32; i < 254; i++)
        {
            idt_set_gate(i, (uint64_t)irq_stub_table[i-32], 0x08, 0x8E);
        }
    irq_routines_table[16]=(isr_t)lapic_timer_handler;

}

void irq_handler(registers_t *regs)
{

    int irq = regs->int_no - 32;
    if (irq_routines_table[irq])
        ((isr_t)(irq_routines_table[irq]))(regs);
    lapic_write(0xB0, 0);
}