#ifndef FLASH_H
#define FLASH_H

#define CONFIG_FLASH_OFFSET 0x101FF000
#define CONFIG_FLASH_LENGTH 4096
#define CONFIG_FLASH_MAGICN 0x5049434F

typedef struct {
    uint32_t magic;
    uint8_t network_ssid[32];
    uint8_t network_pass[64];
} flash_config_t;

_Static_assert(sizeof(flash_config_t) <= CONFIG_FLASH_LENGTH, "flash_config_t exceeds reserved flash sector");

const flash_config_t *flash_get_config(void);

#endif