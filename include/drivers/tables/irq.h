#ifndef _IRQ_H
#define _IRQ_H

#include <drivers/tables/isr.h>

// Standard ISA IRQs
#define IRQ_TIMER       0
#define IRQ_KEYBOARD    1
#define IRQ_CASCADE     2 // Never raised
#define IRQ_COM2        3
#define IRQ_COM1        4
#define IRQ_LPT2        5
#define IRQ_FLOPPYDISK  6
#define IRQ_LPT1        7 // Unreliable
#define IRQ_CMOSCLOCK   8
#define IRQ_FREEDEVICE1 9
#define IRQ_FREEDEVICE2 10
#define IRQ_FREEDEVICE3 11
#define IRQ_PS2MOUSE    12
#define IRQ_FPU         13
#define IRQ_FIRSTATA    14
#define IRQ_SECONDATA   14

extern void irq_install_handler(int irq, isr_t handler);
extern void irq_install();

#endif