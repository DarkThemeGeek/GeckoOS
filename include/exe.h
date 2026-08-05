#pragma once

#include "mem.h"
#include <stddef.h>

void dumpelf(unsigned char* elf, size_t size);
void runelf(Buffer_t elf);