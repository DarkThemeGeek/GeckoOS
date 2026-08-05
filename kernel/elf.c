#include "mem.h"
#include "mem/paging.h"
#include "mem/physical_mem.h"
#include "ports.h"
#include "process/process.h"
#include <stddef.h>
#include <elf.h>
#include <commands.h>
#include <stdint.h>
#include <terminal/printf.h>

void runrawbytes(unsigned char* bytes) {
    void (*func)() = (void*)bytes;
    func();
}

void dumpelf(unsigned char* elf) {
    const Elf64_Ehdr* header = ((const Elf64_Ehdr*)elf);
    
    size_t curl = 0;
    const unsigned char* MagicBytes = header->e_ident;

/*  const unsigned char class = header->e_ident[curl + 4];
    const unsigned char data = header->e_ident[curl + 5];
    const unsigned char version = header->e_ident[curl + 6];
    const unsigned char osabi = header->e_ident[curl + 7];
    const unsigned char abiversion = header->e_ident[curl + 8]; */

    if (!(  MagicBytes[0] == ELFMAG0 &&
            MagicBytes[1] == ELFMAG1 &&
            MagicBytes[2] == ELFMAG2 &&
            MagicBytes[3] == ELFMAG3    )) {
        printf("Invalid ELF file!\n");
        return;
    }
    
    curl = header->e_shoff;

    Elf64_Shdr* section = (Elf64_Shdr*)&elf[curl];
    const unsigned char* string_table = &elf[((Elf64_Shdr*)&elf[curl + (sizeof(Elf64_Shdr) * header->e_shstrndx)])->sh_offset];
    printf("String table is in the section %d\n", header->e_shstrndx);

    for (int i = 0; i < header->e_shnum; i++) {
        curl += sizeof(Elf64_Shdr);
        section = (Elf64_Shdr*)&elf[curl];

        if (i != header->e_shnum - 1) /* The last section will cause a page fault (I think it has to be the section name the cause) */ printf("0x%08X %s %s\n", section->sh_flags, &string_table[section->sh_name], (i == (header->e_shstrndx - 1) ? "(Here is the string table)" : ""));
        if (section->sh_flags & SHF_EXECINSTR) {
            printf("Loadable section\n");
            if (    string_table[section->sh_name + 1] == 't' &&
                    string_table[section->sh_name + 2] == 'e' &&
                    string_table[section->sh_name + 3] == 'x' &&
                    string_table[section->sh_name + 4] == 't'   ) {
                printf("Dumping .text...\n");
                for (size_t y = 0; y < section->sh_size; y++) {
                    printf("%X ", elf[section->sh_offset + y]);
                } printf("\n");
            }
        }
    }
}

void runelf(Buffer_t elf) {
    const Elf64_Ehdr* header = ((const Elf64_Ehdr*)elf.bytes);
    
    size_t curl = 0;
    const unsigned char* MagicBytes = header->e_ident;

/*  const unsigned char class = header->e_ident[curl + 4];
    const unsigned char data = header->e_ident[curl + 5];
    const unsigned char version = header->e_ident[curl + 6];
    const unsigned char osabi = header->e_ident[curl + 7];
    const unsigned char abiversion = header->e_ident[curl + 8]; */

    if (!(  MagicBytes[0] == ELFMAG0 &&
            MagicBytes[1] == ELFMAG1 &&
            MagicBytes[2] == ELFMAG2 &&
            MagicBytes[3] == ELFMAG3    )) {
        printf("Invalid ELF file!\n");
        return;
    }
    
    curl = header->e_shoff;

    Elf64_Shdr* section = (Elf64_Shdr*)&elf.bytes[curl];
    const unsigned char* string_table = &elf.bytes[((Elf64_Shdr*)&elf.bytes[curl + (sizeof(Elf64_Shdr) * header->e_shstrndx)])->sh_offset];

    for (int i = 0; i < header->e_shnum; i++) {
        curl += sizeof(Elf64_Shdr);
        section = (Elf64_Shdr*)&elf.bytes[curl];

        if (section->sh_flags & SHF_EXECINSTR) {
            if (    string_table[section->sh_name + 1] == 't' &&
                    string_table[section->sh_name + 2] == 'e' &&
                    string_table[section->sh_name + 3] == 'x' &&
                    string_table[section->sh_name + 4] == 't'   ) {
                printf("Running .text...\n");
                
                create_process(&elf.bytes[section->sh_offset]);
            }
        }
    }
}