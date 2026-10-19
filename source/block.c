#include "block.h"
#include "hash.h"

static const uint8_t *block_table[BLOCK_TABLE_SIZE];

int block_table_contains(const uint8_t *s) {
    uint32_t s_index = hash_fnv1a(s) % BLOCK_TABLE_SIZE;

    for (size_t i = 0; i < BLOCK_TABLE_SIZE; i++) {
        const uint8_t *entry = block_table[s_index];

        if (entry == NULL) {
            return -1;
        }

        if (strcmp(entry, s) == 0) {
            return 0;
        }

        s_index = (s_index + 1) % BLOCK_TABLE_SIZE;
    }

    return -1;
}

int block_table_insert(const uint8_t *s)
{
    uint32_t s_index = hash_fnv1a(s) % BLOCK_TABLE_SIZE;

    for (size_t i = 0; i < BLOCK_TABLE_SIZE; i++) {
        const uint8_t *entry = block_table[s_index];

        if (entry == NULL) {
            block_table[s_index] = s;
            return 0;
        }

        if (strcmp(entry, s) == 0) return -1;

        s_index = (s_index + 1) % BLOCK_TABLE_SIZE;
    }

    return -1;
}