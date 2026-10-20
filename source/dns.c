#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "lwip/udp.h"
#include "lwip/timeouts.h"
#include "lwip/sys.h"
#include "lwip/ip_addr.h"

#include "dns.h"
#include "common.h"
#include "block.h"

static dns_forward_ctx_t *active_forwards = NULL;
static bool timeout_timer_started = false;

static void start_timeout_timer(void) {
    if (timeout_timer_started) {
        return;
    }

    timeout_timer_started = true;
    sys_timeout(DNS_UPSTREAM_TIMEOUT_MS, dns_timeout_tick, NULL);
}

static void register_forward(dns_forward_ctx_t *ctx) {
    ctx->created_at = sys_now();
    ctx->next = active_forwards;
    active_forwards = ctx;
    start_timeout_timer();
}

static void dns_timeout_tick(void *arg) {
    (void)arg;
    timeout_timer_started = false;
    uint32_t now = sys_now();
    dns_forward_ctx_t *ctx = active_forwards;

    while (ctx != NULL) {
        dns_forward_ctx_t *next = ctx->next;
        if ((uint32_t)(now - ctx->created_at) >= DNS_UPSTREAM_TIMEOUT_MS) {
            printf("DNS upstream timeout after %u ms\n", (unsigned int)(now - ctx->created_at));
            cleanup_forward(ctx);
        }
        ctx = next;
    }

    if (active_forwards != NULL) {
        start_timeout_timer();
    }
}

int parse_header(dns_header_t* h, struct pbuf *buf) {
    if (buf->tot_len < DNS_HEADER_SIZE) {
        printf("Corrupted header (length: %d)\n", buf->tot_len);
        return -1;
    }

    uint8_t raw[DNS_HEADER_SIZE];

    if (pbuf_copy_partial(buf, raw, sizeof(raw), 0) != sizeof(raw)) {
        printf("Failed to copy header bytes\n");
        return -1;
    }

    h->id = read_u16_be(&raw[0]);
    h->details = read_u16_be(&raw[2]);
    h->qdcount = read_u16_be(&raw[4]);
    h->ancount = read_u16_be(&raw[6]);
    h->nscount = read_u16_be(&raw[8]);
    h->arcount = read_u16_be(&raw[10]);

    printf("DNS ID: %u\n", h->id);
    printf("QDCOUNT=%u ANCOUNT=%u NSCOUNT=%u ARCOUNT=%u\n", h->qdcount, h->ancount, h->nscount, h->arcount);

    return 0;
}

void parse_next_question(struct pbuf *buf, int *read_offset, uint8_t *record) {
    int first_label = 1;
    int record_offset = 0;
    uint8_t octet_buffer;

    while (1) {
        pbuf_copy_partial(buf, &octet_buffer, sizeof(uint8_t), *read_offset);
        (*read_offset)++;
        if ((octet_buffer & DNS_COMPRESSION_BYTE) == DNS_COMPRESSION_BYTE) {
            // todo: support compressed questions
            (*read_offset)++;
            break;
        }
        if (octet_buffer==0) break;
        int label_length = octet_buffer;
        if (!first_label) {
            record[record_offset++] = '.';
        }

        for (int i = 0 ; i < label_length ; i++) {
            pbuf_copy_partial(buf, &octet_buffer, sizeof(uint8_t), *read_offset);
            (*read_offset)++;
            record[record_offset++] = octet_buffer;
        }

        first_label = false;
    }
    record[record_offset] = '\0';

    uint8_t qtype[2];
    pbuf_copy_partial(buf, qtype, sizeof(qtype), *read_offset);
    (*read_offset)+=2;
    uint16_t qtype_value = read_u16_be(&qtype[0]);
    printf("QTYPE: %u\n", qtype_value);

    uint8_t qclass[2];
    pbuf_copy_partial(buf, qclass, sizeof(qclass), *read_offset);
    (*read_offset)+=2;
    uint16_t qclass_value = read_u16_be(&qclass[0]);
    printf("QCLASS: %u\n", qclass_value);
}

void cleanup_forward(dns_forward_ctx_t *ctx) {
    // if (ctx == NULL) {
    //     return;
    // }

    // sys_untimeout(dns_forward_timeout, ctx);

    // if (ctx->upstream_pcb != NULL) {
    //     udp_recv(ctx->upstream_pcb, NULL, NULL);
    //     udp_remove(ctx->upstream_pcb);
    //     ctx->upstream_pcb = NULL;
    // }

    // free(ctx);

    if (ctx == NULL) {
        return;
    }

    dns_forward_ctx_t **current = &active_forwards;

    while (*current != NULL) {
        if (*current == ctx) {
            *current = ctx->next;
            break;
        }
        current = &(*current)->next;
    }

    if (ctx->upstream_pcb != NULL) {
        udp_recv(ctx->upstream_pcb, NULL, NULL);
        udp_remove(ctx->upstream_pcb);
        ctx->upstream_pcb = NULL;
    }

    free(ctx);
}

void dns_upstream_hander(void *arg, struct udp_pcb *pcb, struct pbuf *buf, const ip_addr_t* addr, uint16_t port) {
    dns_forward_ctx_t *ctx = (dns_forward_ctx_t *)arg;
    if (buf == NULL) {
        printf("DNS upstream returned NULL packet\n");
        return;
    }

    if (ctx == NULL) {
        pbuf_free(buf);
        return;
    }

    if (!ip_addr_cmp(addr, &upstream_dns) || port != 53) {
        printf("Ignoring unexpected DNS response\n");
        pbuf_free(buf);
        return;
    }

    err_t err = udp_sendto(ctx->client_pcb, buf, &ctx->client_addr, ctx->client_port);

    if (err != ERR_OK) {
        printf("DNS response forwarding failed: err_t=%d\n", (int)err);
    }

    pbuf_free(buf);
    cleanup_forward(ctx);
}

int dns_forward_query(struct udp_pcb *pcb, struct pbuf *buf, const ip_addr_t* addr, uint16_t port) {
    dns_forward_ctx_t *ctx = calloc(1, sizeof(*ctx));
    if (ctx == NULL) {
        printf("Failed to allocate DNS context\n");
        return -1;
    }

    ctx->client_pcb = pcb;
    ctx->client_addr = *addr;
    ctx->client_port = port;

    ctx->upstream_pcb = udp_new();

    if (ctx->upstream_pcb == NULL) {
        printf("udp_new failed: returned NULL (PCB allocation failure)\n");
        free(ctx);
        return -1;
    }

    udp_recv(ctx->upstream_pcb, dns_upstream_hander, ctx);

    register_forward(ctx);

    err_t err = udp_sendto(ctx->upstream_pcb, buf, &upstream_dns, 53);

    if (err != ERR_OK) {
        printf("DNS upstream udp_sendto failed: err_t=%d\n", (int)err);
        cleanup_forward(ctx);
        return -1;
    }

    return 0;
}

void dns_message_handler(void* arg, struct udp_pcb *pcb, struct pbuf *buf, const ip_addr_t* addr, uint16_t port) {
    printf("\n\n");
    if (buf == NULL) {
        printf("Empty UDP packet\n");
        return;
    }
    printf("UDP packet: %u bytes, source port %u\n", (unsigned int)buf->tot_len, (unsigned int)port);

    dns_header_t header;

    if (parse_header(&header, buf)) {
        printf("Error parsing header\n");
        pbuf_free(buf);
        return;
    }

    int read_offset = DNS_HEADER_SIZE;
    int present_on_blocklist = -1;

    for (size_t i = 0 ; i < header.qdcount; i++) {
        uint8_t record[256] = {0};

        parse_next_question(buf, &read_offset, record);

        printf("Q: %s\n", (char *)record);

        present_on_blocklist = block_table_contains(record);

        if (!present_on_blocklist) break;
    }

    if (!present_on_blocklist) {
        printf("Blocked DNS query; returning 0.0.0.0\n");
        if (dns_send_blocked_response(pcb, buf, addr, port) != 0) {
            printf("Failed to send blocked DNS response\n");
        }
    } else {
        dns_forward_query(pcb, buf, addr, port);
    }

    pbuf_free(buf);
}

int dns_send_blocked_response(struct udp_pcb *pcb, struct pbuf *query, const ip_addr_t *client_addr, uint16_t client_port) {
    if (pcb == NULL || query == NULL || client_addr == NULL) {
        return -1;
    }

    uint8_t header[DNS_HEADER_SIZE];

    if (query->tot_len < DNS_HEADER_SIZE ||
        pbuf_copy_partial(query, header, sizeof(header), 0) != sizeof(header)) {
        return -1;
    }

    uint16_t qdcount = read_u16_be(&header[4]);
    if (qdcount != 1) {
        printf("Cannot synthesize DNS response: QDCOUNT != 1\n");
        return -1;
    }

    uint16_t flags = read_u16_be(&header[2]);

    if ((flags & 0x8000u) != 0) {
        return -1;
    }

    uint16_t query_len = query->tot_len;
    uint16_t offset = DNS_HEADER_SIZE;
    uint8_t label_len;

    while (1) {
        if (offset >= query_len ||
            pbuf_copy_partial(query, &label_len, 1, offset) != 1) {
            return -1;
        }

        offset++;

        if ((label_len & DNS_COMPRESSION_BYTE) == DNS_COMPRESSION_BYTE) {
            if (offset >= query_len) {
                return -1;
            }

            offset++;
            break;
        }

        if ((label_len & DNS_COMPRESSION_BYTE) != 0) {
            return -1;
        }

        if (label_len == 0) {
            break;
        }

        if (label_len > 63 ||
            (uint32_t)offset + label_len > query_len) {
            return -1;
        }

        offset += label_len;
    }

    if ((uint32_t)offset + 4 > query_len) {
        return -1;
    }

    uint8_t question_tail[4];

    if (pbuf_copy_partial(
            query, question_tail, sizeof(question_tail), offset
        ) != sizeof(question_tail)) {
        return -1;
    }

    uint16_t qtype = read_u16_be(&question_tail[0]);
    uint16_t qclass = read_u16_be(&question_tail[2]);

    if (qtype != 1 || qclass != 1) {
        printf("Blocked query is not type A / class IN\n");
        return -1;
    }

    uint16_t question_end = offset + 4;
    uint16_t answer_len = 16;
    uint16_t response_len = question_end + answer_len;

    uint8_t response[512];

    if (response_len > sizeof(response)) {
        return -1;
    }

    if (pbuf_copy_partial(
            query, response, question_end, 0
        ) != question_end) {
        return -1;
    }

    uint16_t response_flags = (flags & 0x0100u) | 0x8080u;

    response[2] = (uint8_t)(response_flags >> 8);
    response[3] = (uint8_t)response_flags;

    response[6] = 0;
    response[7] = 1;
    response[8] = 0;
    response[9] = 0;
    response[10] = 0;
    response[11] = 0;

    uint8_t answer[] = {
        0xC0, 0x0C,             // NAME
        0x00, 0x01,             // TYPE = A
        0x00, 0x01,             // CLASS = IN
        0x00, 0x00, 0x00, 0x3C, // TTL = 60 seconds
        0x00, 0x04,             // RDLENGTH = 4
        0x00, 0x00, 0x00, 0x00  // RDATA = 0.0.0.0
    };

    memcpy(response + question_end, answer, sizeof(answer));

    struct pbuf *reply = pbuf_alloc(
        PBUF_TRANSPORT, response_len, PBUF_RAM
    );

    if (reply == NULL) {
        printf("Failed to allocate blocked DNS response\n");
        return -1;
    }

    err_t err = pbuf_take(reply, response, response_len);

    if (err == ERR_OK) {
        err = udp_sendto(pcb, reply, client_addr, client_port);
    }

    if (err != ERR_OK) {
        printf("Blocked DNS response send failed: err_t=%d\n", (int)err);
    }

    pbuf_free(reply);

    return err == ERR_OK ? 0 : -1;
}