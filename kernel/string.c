#include <stdint.h>
#include <string.h>

void strcpy(char *d, const char *s)
{
    int i = 0;
    while (s[i]) {
        d[i] = s[i];
        i++;
    }
    d[i] = 0;
}
bool strcmp(const char *a, const char *b)
{
    int i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i])
            return false;
        i++;
    }
    return a[i] == b[i];
}
int strlen(const char *s)
{
    int n = 0;
    while (s[n])
        n++;
    return n;
}
int memcmp(char *a, char *b, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        if (a[i] < b[i])
            return -1;
        else if (a[i] > b[i])
            return 1;
    }
    return 0;
}
char *strstr(char *haystack, char *needle)
{
    int i, j;

    if (*needle == 0)
        return haystack;

    for (i = 0; haystack[i] != 0; i++) {
        for (j = 0; needle[j] != 0; j++) {
            if (haystack[i + j] != needle[j])
                break;
        }

        if (needle[j] == 0)
            return &haystack[i];
    }

    return 0;
}