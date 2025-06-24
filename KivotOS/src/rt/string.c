//
// Created by neko on 6/7/25.
//

#include <stdlib.h>
#include <string.h>
#include <x86/log.h>
#include <stdint.h>

size_t strlen (const char *s) {
    size_t len = 0;
    while (*s) {
        len++;
        s++;
    }
    return len;
}


int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2) {
        if (*s1 != *s2)
            return (unsigned char)*s1 - (unsigned char)*s2;
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++)) {}
    return dest;
}

char *strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d) d++;
    while ((*d++ = *src++)) {}
    return dest;
}

char *strdup (const char *__s) {
    char* d = malloc(strlen(__s) + 1);
    memset(d, 0, strlen(__s) + 1);
    strcpy(d, __s);
    return d;
}
