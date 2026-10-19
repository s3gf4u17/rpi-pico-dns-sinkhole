#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "flash.h"

int initialize_networking() {
    const flash_config_t *config = flash_get_config();
    if (config == NULL) {
        printf("Failed to load config\n");
        return -1;
    }
    if (cyw43_arch_init()) {
        printf("Failed to initialize CYW43 architecture\n");
        return -1;
    }
    cyw43_arch_enable_sta_mode();
    const int retry_limit = 5;
    int retry_counter = 0;
    while (retry_counter < retry_limit) {
        printf("Connecting to network...\n");
        if (!cyw43_arch_wifi_connect_timeout_ms(
            config->network_ssid,
            config->network_pass,
            CYW43_AUTH_WPA2_AES_PSK,
            5000)
        ) break;
        sleep_ms(10000);
        retry_counter++;
    }
    if (retry_counter >= retry_limit) {
        printf("Failed to connect to network\n");
        return -1;
    }
    printf("Pico IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
}

int run_udp_server() {
    cyw43_arch_lwip_begin();

    struct udp_pcb *server = udp_new();

    if (server==NULL) {
        cyw43_arch_lwip_end();
        printf("Failed to create new UDP server");
        return -1;
    }

    err_t result = udp_bind(server, IP_ANY_TYPE, 53);

    if (result != ERR_OK) {
        udp_remove(server);
        cyw43_arch_lwip_end();
        printf("Failed to bind UDP server");
        return -1;
    }

    // udp_recv(server, ..., NULL);

    cyw43_arch_lwip_end();

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, true);
}

int main() {
    stdio_init_all();

    if (initialize_networking()) {
        printf("Failed to initialize networking\n");
        return -1;
    }

    if (run_udp_server()) {
        printf("Failed to prepare UDP server\n");
        return -1;
    }

    while (1) { sleep_ms(1000); }
}