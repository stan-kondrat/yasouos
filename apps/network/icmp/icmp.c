#include "icmp.h"
#include "../net_utils.h"

static uint16_t icmp_checksum(const icmp_hdr_t *header, size_t length) {
    if (length < sizeof(icmp_hdr_t)) {
        return 0;
    }

    uint32_t sum = 0;
    const uint16_t *ptr = (const uint16_t *)header;

    for (size_t i = 0; i < sizeof(icmp_hdr_t) / 2; i++) {
        sum += ntohs(ptr[i]);
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return htons(~sum);
}

void icmp_print(const uint8_t *packet, size_t length, int leftpad) {
    if (!packet || length < sizeof(icmp_hdr_t)) {
        puts("[ICMP] Packet too small\n");
        return;
    }

    const icmp_hdr_t *icmp = (const icmp_hdr_t *)packet;
    uint16_t id = ntohs_unaligned(&icmp->id);
    uint16_t seq = ntohs_unaligned(&icmp->sequence);

    for (int i = 0; i < leftpad; i++) {
        putchar(' ');
    }
    puts("[ICMP] type=");
    net_print_decimal_u8(icmp->type);
    puts(" code=");
    net_print_decimal_u8(icmp->code);
    puts(" id=");
    net_print_decimal_u16(id);
    puts(" seq=");
    net_print_decimal_u16(seq);
    puts(" len=");
    net_print_decimal_u8(length);
    puts("\n");
}

void icmp_build_request(icmp_hdr_t *header, uint16_t id, uint16_t sequence) {
    header->type = ICMP_ECHO_REQUEST;
    header->code = 0;
    header->checksum = 0;
    header->id = htons(id);
    header->sequence = htons(sequence);
    header->checksum = icmp_checksum(header, sizeof(icmp_hdr_t));
}

void icmp_build_response(icmp_hdr_t *header, uint16_t id, uint16_t sequence) {
    header->type = ICMP_ECHO_REPLY;
    header->code = 0;
    header->checksum = 0;
    header->id = htons(id);
    header->sequence = htons(sequence);
    header->checksum = icmp_checksum(header, sizeof(icmp_hdr_t));
}

bool icmp_parse(const uint8_t *packet, size_t length, uint8_t *type, uint8_t *code, uint16_t *id, uint16_t *sequence) {
    if (!packet || length < sizeof(icmp_hdr_t)) {
        return false;
    }

    const icmp_hdr_t *icmp = (const icmp_hdr_t *)packet;

    if (type) {
        *type = icmp->type;
    }
    if (code) {
        *code = icmp->code;
    }
    if (id) {
        *id = ntohs_unaligned(&icmp->id);
    }
    if (sequence) {
        *sequence = ntohs_unaligned(&icmp->sequence);
    }

    return true;
}
