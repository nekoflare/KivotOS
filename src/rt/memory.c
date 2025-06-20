//
// Created by neko on 6/7/25.
//

#include <string.h>

void *memmove(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *) dest;
    const unsigned char *s = (const unsigned char *) src;

    if (d < s) {
        for (size_t i = 0; i < n; i++) {
            d[i] = s[i];
        }
    } else {
        for (size_t i = n; i > 0; i--) {
            d[i - 1] = s[i - 1];
        }
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = (const unsigned char *) s1;
    const unsigned char *p2 = (const unsigned char *) s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }

    return 0;
}


void *memcpy(void *__restrict dest, const void *__restrict src, size_t n) {
    char* d = (char*)dest;
    const char* s = (const char*)src;

    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }

    return dest;
}


void *memset(void *s, int c, size_t n) {
    unsigned char* p = (unsigned char*) s;
    unsigned char byte = (unsigned char) c;

    for (size_t i = 0; i < n; i++) {
        p[i] = byte;
    }

    return s;
}