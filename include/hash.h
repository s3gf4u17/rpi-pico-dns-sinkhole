#ifndef HASH_H
#define HASH_H

#include <stdint.h>

static uint32_t hash_fnv1a(const uint8_t *s) {
    uint32_t hash = 2166136261u;

    while (*s) {
        hash ^= *s++;
        hash *= 16777619u;
    }

    return hash;
}

#endif