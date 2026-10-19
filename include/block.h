#ifndef BLOCK_H
#define BLOCK_H

#include <string.h>
#include <stdint.h>

#define BLOCK_TABLE_SIZE 64

int block_table_contains(const uint8_t *s);

int block_table_insert(const uint8_t *s);

#endif