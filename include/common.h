#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

static uint16_t read_u16_be(const uint8_t *buf) {
    return ((uint16_t)buf[0] << 8) | buf[1];
}

#endif