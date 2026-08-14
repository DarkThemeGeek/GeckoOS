#ifndef _IRQ_H
#define _IRQ_H

#include <drivers/tables/isr.h>

void irq_install_handler(int irq, isr_t handler,int flags);
extern void irq_install();

#endif