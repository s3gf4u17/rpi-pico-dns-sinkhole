#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "flash.h"

static flash_config_t config;

static uint8_t config_loaded = 0;

int flash_load_config() {
    if (config_loaded) {
        return 0;
    }
    const flash_config_t* buffer = (const flash_config_t*)CONFIG_FLASH_OFFSET;
    if (buffer->magic != CONFIG_FLASH_MAGICN) {
        printf("Config magic number mismatch: 0x%X != 0x%X\n", buffer->magic, CONFIG_FLASH_MAGICN);
        return -1;
    }
    memcpy(&config, buffer, sizeof(flash_config_t));
    config.network_ssid[sizeof(config.network_ssid) - 1] = '\0';
    config.network_pass[sizeof(config.network_pass) - 1] = '\0';
    config_loaded = 1;
    printf("Config loaded\n");
    return 0;
}

const flash_config_t *flash_get_config(void) {
    if (!config_loaded) {
        if (flash_load_config()) {
            printf("Error loading config file\n");
            return NULL;
        }
    }
    return &config;
}