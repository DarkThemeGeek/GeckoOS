#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void strcpy(char *d, const char *s);
bool strcmp(const char *a, const char *b);
int strlen(const char *s);
int memcmp(char *a, char *b, size_t size);
char *strstr(char *haystack, char *needle);