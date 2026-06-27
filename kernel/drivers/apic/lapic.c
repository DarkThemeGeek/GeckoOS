#include "drivers/apic/lapic.h"
#include "drivers/acpi.h"
#include "drivers/tables/idt.h"
#include "mem/paging.h"
#include "ports.h"
#include "terminal/terminal.h"
#include <stdbool.h>
#include <stdint.h>
#include <terminal/printf.h>
#include "drivers/tables/irq.h"
#define IA32_APIC_BASE_MSR        0x1B
#define IA32_APIC_BASE_MSR_ENABLE (1ULL << 11)
#define IA32_APIC_BASE_MSR_BSP    (1ULL << 8)
// flags for checking if cpu has apic
#define CPUID_FEAT_EDX_APIC   (1 << 9)
#define CPUID_FEAT_ECX_X2APIC (1 << 21)



extern volatile uint32_t *lapic;

volatile uint32_t *lapic;
extern uintptr_t acpi_lapic_base;
//model specific register acess read
static inline uint64_t rdmsr(uint32_t msr)
{
    uint32_t lo, hi;
    __asm__ volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(msr));
    return ((uint64_t)hi << 32) | lo;
}
//model specific register acess write
static inline void wrmsr(uint32_t msr, uint64_t value)
{
    uint32_t lo = (uint32_t)value;
    uint32_t hi = (uint32_t)(value >> 32);
    __asm__ volatile("wrmsr" : : "c"(msr), "a"(lo), "d"(hi));
}
//getting cpuid
static inline void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t *eax,
                         uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
    __asm__ volatile("cpuid"
                     : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                     : "a"(leaf), "c"(subleaf));
}
//checkin the cpuid for the bit that shows that cpu has apic
bool cpu_has_apic()
{
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    bool has_apic = edx & CPUID_FEAT_EDX_APIC;
    return has_apic;
}
//checking for the more modern 32bit reg apic
bool cpu_has_x2apic()
{
    uint32_t eax, ebx, ecx, edx;
    cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    bool has_x2apic = ecx & CPUID_FEAT_ECX_X2APIC;
    return has_x2apic;
}
//lapic id for writing to ioapic x2apic
uint32_t lapic_get_id(void) { return (lapic_read(LAPIC_REG_ID) >> 24) & 0xFF; }

//getting the lapic id with msr this wat for the v1 apic
uint32_t lapic_get_id_msr(void)
{
    if (cpu_has_x2apic()) {
        // In x2APIC mode, LAPIC ID is in MSR 0x802
        return (uint32_t)rdmsr(0x802);
    } else {
        // xAPIC mode - read from MMIO
        return lapic_get_id();
    }
}
//the timer
void lapic_timer_start(void) {
    // Stop the timer first
    lapic_write(LAPIC_REG_TIMER_ICR, 0);

    // Set divider: divide by 16 (0x3 is typical) (frequenct)
    lapic_write(LAPIC_REG_TIMER_DCR, 0x03);

    // Set LVT: vector 48, periodic mode (bit 17 set)
    // bit 16 = 0 (unmasked)
    uint32_t lvt = 48 | (1 << 17);
    lapic_write(LAPIC_REG_TIMER_LVT, lvt);

        
    // Set initial count
    lapic_write(LAPIC_REG_TIMER_ICR, 0x100000);

    //irq_install_handler(48, lapic_timer_handler, 0);
}

extern uint64_t lapic_timer_tick;
uint64_t lapic_timer_tick=0;
//works finally
void lapic_timer_handler(registers_t *regs) {
    lapic_timer_tick++;  

}
void cpu_set_apic_base(uint64_t apic)
{
    uint64_t value;

    value = apic & 0xFFFFF000ULL;
    value |= IA32_APIC_BASE_MSR_ENABLE;

    wrmsr(IA32_APIC_BASE_MSR, value);

}

uint64_t cpu_get_apic_base(void)
{
    uint64_t value = rdmsr(IA32_APIC_BASE_MSR);
    return value & 0xFFFFF000ULL;
}

volatile uint32_t *lapic;
extern uintptr_t acpi_lapic_base;

uint32_t lapic_read(uint32_t reg) { return lapic[reg / 4]; }

void lapic_write(uint32_t reg, uint32_t value) { lapic[reg / 4] = value; }

volatile uint32_t *get_lapic_base() { return lapic; }

int lapic_init() {
    if (!cpu_has_apic()) {
        return -1;
    }

    // Disable PIC
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);

    // Map LAPIC
    uintptr_t lapic_phys = cpu_get_apic_base();
    if (acpi_lapic_base != 0) {
        lapic_phys = acpi_lapic_base;
    }
    
    // Identity map (or map to a known virtual address)
    uintptr_t lapic_virt = lapic_phys;
    vmm_map(vmm_get_pml4(), lapic_phys, lapic_virt,
            PTE_WRITABLE | PTE_CACHE_DISABLE);
    
    lapic = (volatile uint32_t *)lapic_virt;

    // Enable APIC via MSR
    uint64_t msr = rdmsr(IA32_APIC_BASE_MSR);
    msr |= IA32_APIC_BASE_MSR_ENABLE;
    wrmsr(IA32_APIC_BASE_MSR, msr);

    // Set SVR - Enable APIC with spurious vector
    uint32_t svr = lapic_read(LAPIC_REG_SVR);
     //svr &= ~0xFF;  // Clear vector
     //svr |= 0xFF;   // Spurious vector 255
     svr |= (1 << 8);  // APIC enable
    //svr |= 0x100;
    lapic_write(LAPIC_REG_SVR,  0x100 | 0xEF);
    // Set TPR to accept all interrupts
    lapic_write(LAPIC_REG_TPR, 0);

    // Mask LINT0 and LINT1
    lapic_write(LAPIC_REG_LINT0_LVT, (1 << 16));  // Masked
    lapic_write(LAPIC_REG_LINT1_LVT, (1 << 16));  // Masked

    // Mask timer initially
    lapic_write(LAPIC_REG_TIMER_LVT, (1 << 16));

    // Clear error status
    lapic_write(LAPIC_REG_ESR, 0);
    lapic_write(LAPIC_REG_ESR, 0);

    // Send EOI
    lapic_write(LAPIC_REG_EOI, 0);

    // Verify
    uint32_t id = lapic_get_id();
    uint32_t ver = lapic_read(LAPIC_REG_VERSION);
    //printf("LAPIC initialized:Adress%x ID=%u, Version=0x%x\n",lapic, id, ver);
    
    return 0;
}

