#ifndef DNS_H
#define DNS_H

#define DNS_COMPRESSION_BYTE 0b11000000
#define DNS_HEADER_SIZE 12
#define DNS_MAX_NAME_TEXT 254
#define DNS_UPSTREAM_TIMEOUT_MS 3000

typedef struct {
    uint16_t id;
    uint16_t details;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} dns_header_t;

typedef struct dns_forward_ctx {
    struct udp_pcb *client_pcb;
    struct udp_pcb *upstream_pcb;
    ip_addr_t client_addr;
    uint16_t client_port;
    uint32_t created_at;
    struct dns_forward_ctx *next;
} dns_forward_ctx_t;

static void dns_timeout_tick(void *arg);

static void dns_forward_timeout(void *arg);

void cleanup_forward(dns_forward_ctx_t *ctx);

void dns_message_handler(void* arg, struct udp_pcb *pcb, struct pbuf *buf, const ip_addr_t* addr, uint16_t port);

int dns_send_blocked_response(struct udp_pcb *pcb, struct pbuf *query, const ip_addr_t *client_addr, uint16_t client_port);

static const ip_addr_t upstream_dns = IPADDR4_INIT_BYTES(192, 168, 1, 1);

#endif