#include "http_hello.h"
#include "../netdev-mac/netdev.h"
#include "../network/net_utils.h"
#include "../network/ethernet/ethernet.h"
#include "../network/arp/arp.h"
#include "../network/ipv4/ipv4.h"
#include "../network/tcp/tcp.h"
#include "../../common/common.h"
#include "../../common/byteorder.h"
#include "../../common/log.h"

static log_tag_t *http_log;

// Write a decimal u8 to buffer, return number of bytes written
static size_t write_decimal_u8(uint8_t *buf, uint8_t value) {
    size_t len = 0;
    if (value >= 100) {
        buf[len++] = '0' + (value / 100);
        value %= 100;
        buf[len++] = '0' + (value / 10);
        buf[len++] = '0' + (value % 10);
    } else if (value >= 10) {
        buf[len++] = '0' + (value / 10);
        buf[len++] = '0' + (value % 10);
    } else {
        buf[len++] = '0' + value;
    }
    return len;
}

// Write "Hello, A.B.C.D\n" to buf, return length
static size_t write_hello_ip(uint8_t *buf, uint32_t ip_host) {
    size_t len = 0;
    buf[len++] = 'H'; buf[len++] = 'e'; buf[len++] = 'l';
    buf[len++] = 'l'; buf[len++] = 'o'; buf[len++] = ',';
    buf[len++] = ' ';
    len += write_decimal_u8(buf + len, (ip_host >> 24) & 0xFF);
    buf[len++] = '.';
    len += write_decimal_u8(buf + len, (ip_host >> 16) & 0xFF);
    buf[len++] = '.';
    len += write_decimal_u8(buf + len, (ip_host >> 8) & 0xFF);
    buf[len++] = '.';
    len += write_decimal_u8(buf + len, ip_host & 0xFF);
    buf[len++] = '\n';
    return len;
}

// Write Content-Length value to buf, return length
static size_t write_decimal_size(uint8_t *buf, size_t value) {
    size_t len = 0;
    if (value == 0) { buf[len++] = '0'; return len; }
    uint8_t digits[4];
    int n = 0;
    while (value > 0) { digits[n++] = '0' + (value % 10); value /= 10; }
    while (n > 0) { buf[len++] = digits[--n]; }
    return len;
}

// Build full HTTP response into buf, return total length
static size_t build_http_response(uint8_t *buf, uint32_t client_ip_host) {
    // Build body first to know Content-Length
    uint8_t body[32];
    size_t body_len = write_hello_ip(body, client_ip_host);

    // Use volatile to prevent GCC -O3 from coalescing byte writes into
    // unaligned 32-bit stores (causes Data Abort on ARM64 with SCTLR.A)
    volatile uint8_t *dst = buf;
    size_t len = 0;
    #define APPEND(s) do { \
        const char *_s = (s); \
        while (*_s) dst[len++] = *_s++; \
    } while(0)

    APPEND("HTTP/1.1 200 OK\r\nConnection: keep-alive\r\nContent-Type: text/plain\r\nContent-Length: ");
    len += write_decimal_size(buf + len, body_len);
    APPEND("\r\n\r\n");
    for (size_t i = 0; i < body_len; i++) dst[len++] = body[i];

    #undef APPEND
    return len;
}

static void send_tcp_packet(const device_entry_t *dev, uint8_t *reply_buffer,
                            const uint8_t *our_mac, const uint8_t *their_mac,
                            uint32_t src_ip_net, uint32_t dst_ip_net,
                            uint16_t src_port, uint16_t dst_port,
                            uint32_t seq, uint32_t ack,
                            uint8_t flags, uint16_t window,
                            const uint8_t *payload, uint16_t payload_len) {
    // Ethernet header
    // Use volatile to prevent GCC -O3 from coalescing byte writes into
    // unaligned 32-bit stores (reply_buffer is 2-byte aligned due to
    // ETH_ALIGNMENT_OFFSET, causing Data Abort on ARM64 with SCTLR.A)
    volatile uint8_t *dst = reply_buffer;
    for (int i = 0; i < 6; i++) {
        dst[i] = their_mac[i];
        dst[6 + i] = our_mac[i];
    }
    eth_hdr_t *eth = (eth_hdr_t *)reply_buffer;
    write_htons_unaligned(&eth->type, ETH_P_IP);

    // IPv4 header
    ipv4_hdr_t *ip = (ipv4_hdr_t *)(reply_buffer + sizeof(eth_hdr_t));
    ipv4_build_header(ip, src_ip_net, dst_ip_net, IPPROTO_TCP,
                      sizeof(tcp_hdr_t) + payload_len, 64);

    // Copy payload after TCP header (before checksum calculation)
    // Use volatile to prevent GCC -O3 from widening byte copies into
    // unaligned 32-bit stores on the 2-byte aligned reply buffer
    tcp_hdr_t *tcp = (tcp_hdr_t *)(reply_buffer + sizeof(eth_hdr_t) + sizeof(ipv4_hdr_t));
    volatile uint8_t *tcp_payload = reply_buffer + sizeof(eth_hdr_t) + sizeof(ipv4_hdr_t) + sizeof(tcp_hdr_t);
    for (uint16_t i = 0; i < payload_len; i++) {
        tcp_payload[i] = payload[i];
    }

    // Build TCP header (computes checksum over header + payload)
    tcp_build_header(tcp, src_port, dst_port, seq, ack, flags, window,
                     src_ip_net, dst_ip_net, payload_len);

    size_t total_len = sizeof(eth_hdr_t) + sizeof(ipv4_hdr_t) + sizeof(tcp_hdr_t) + payload_len;
    netdev_transmit(dev, reply_buffer, total_len);
}

void app_http_hello(void) {
    http_log = log_register("http-hello", LOG_INFO);
    log_info(http_log, "Starting HTTP Hello World application...\n");
    device_entry_t devices[1] = {0};
    int device_count = netdev_acquire_all(devices, 1);

    if (device_count < 1) {
        log_error(http_log, "No network devices found\n");
        return;
    }

    log_debug(http_log, "Initializing network device...\n");

    uint8_t mac[6];
    int result = netdev_get_mac(&devices[0], mac);

    if (result == 0 && log_enabled(http_log, LOG_INFO)) {
        log_prefix(http_log, LOG_INFO);
        puts("MAC: ");
        net_print_mac(mac);
        puts("\n");
    }

    if (log_enabled(http_log, LOG_INFO)) {
        log_prefix(http_log, LOG_INFO);
        puts("Listening on port ");
        net_print_decimal_u16(HTTP_HELLO_PORT);
        puts("...\n");
    }

    // Ethernet frame alignment
    #define ETH_ALIGNMENT_OFFSET 2
    uint8_t buffer_storage[HTTP_HELLO_BUFFER_SIZE + ETH_ALIGNMENT_OFFSET] __attribute__((aligned(4)));
    uint8_t reply_buffer_storage[HTTP_HELLO_BUFFER_SIZE + ETH_ALIGNMENT_OFFSET] __attribute__((aligned(4)));
    uint8_t *buffer = buffer_storage + ETH_ALIGNMENT_OFFSET;
    uint8_t *reply_buffer = reply_buffer_storage + ETH_ALIGNMENT_OFFSET;

    // Stateless TCP: use a monotonic counter for SYN-ACK ISN, then derive
    // our seq from the client's ack_num in subsequent packets (the client
    // echoes back what it expects from us — no per-connection state needed).
    static uint32_t isn_counter = 1000;

    while (1) {
        size_t received_length = 0;
        result = netdev_receive(&devices[0], buffer, HTTP_HELLO_BUFFER_SIZE, &received_length);

        if (result != 0 || received_length == 0) {
            continue;
        }

        if (received_length < sizeof(eth_hdr_t)) {
            continue;
        }

        const eth_hdr_t *eth = (const eth_hdr_t *)buffer;
        uint16_t eth_type = ntohs_unaligned(&eth->type);

        // Handle ARP requests — reply to any IP
        if (eth_type == ETH_P_ARP) {
            const arp_hdr_t *arp_req = (const arp_hdr_t *)(buffer + sizeof(eth_hdr_t));
            uint16_t opcode = ntohs_unaligned(&arp_req->opcode);

            if (opcode == ARP_OP_REQUEST) {
                uint32_t target_ip = ntohl_unaligned(&arp_req->target_ip);
                uint32_t sender_ip = ntohl_unaligned(&arp_req->sender_ip);
                arp_build_reply(reply_buffer, mac, target_ip,
                               arp_req->sender_mac, sender_ip);
                netdev_transmit(&devices[0], reply_buffer, ARP_PACKET_SIZE);
                log_debug(http_log, "Sent ARP reply\n");
            }
            continue;
        }

        if (eth_type != ETH_P_IP) {
            continue;
        }

        if (received_length < sizeof(eth_hdr_t) + sizeof(ipv4_hdr_t) + sizeof(tcp_hdr_t)) {
            continue;
        }

        const ipv4_hdr_t *ip = (const ipv4_hdr_t *)(buffer + sizeof(eth_hdr_t));
        if (ip->protocol != IPPROTO_TCP) {
            continue;
        }

        const tcp_hdr_t *tcp = (const tcp_hdr_t *)(buffer + sizeof(eth_hdr_t) + sizeof(ipv4_hdr_t));
        uint16_t dst_port = ntohs_unaligned(&tcp->dst_port);
        if (dst_port != HTTP_HELLO_PORT) {
            continue;
        }

        uint16_t src_port = ntohs_unaligned(&tcp->src_port);
        uint32_t their_seq = ntohl_unaligned(&tcp->seq_num);
        uint32_t their_ack = ntohl_unaligned(&tcp->ack_num);
        uint8_t flags = tcp->flags;
        uint8_t data_offset = (tcp->data_offset >> 4) * 4;
        uint16_t ip_total_len = ntohs_unaligned(&ip->total_length);
        uint16_t tcp_payload_len = ip_total_len - sizeof(ipv4_hdr_t) - data_offset;

        if (log_enabled(http_log, LOG_DEBUG)) {
            ethernet_print(buffer, received_length, devices[0].resource, 0);
        }

        // SYN → reply SYN+ACK with fresh ISN
        if (flags & TCP_FLAG_SYN) {
            log_debug(http_log, "SYN received\n");

            uint32_t our_isn = isn_counter++;

            send_tcp_packet(&devices[0], reply_buffer, mac, eth->src,
                           ip->dst_ip, ip->src_ip,
                           dst_port, src_port,
                           our_isn, their_seq + 1,
                           TCP_FLAG_SYN | TCP_FLAG_ACK, 65535,
                           NULL, 0);

            log_debug(http_log, "Sent SYN+ACK\n");
        }

        // Data arrived → reply with HTTP response (keep-alive, no FIN)
        // Use their_ack as our seq (client tells us what it expects)
        if (tcp_payload_len > 0) {
            log_debug(http_log, "HTTP request received, sending response\n");

            uint32_t client_ip = ntohl_unaligned(&ip->src_ip);
            uint8_t http_buf[192];
            size_t http_len = build_http_response(http_buf, client_ip);

            send_tcp_packet(&devices[0], reply_buffer, mac, eth->src,
                           ip->dst_ip, ip->src_ip,
                           dst_port, src_port,
                           their_ack, their_seq + tcp_payload_len,
                           TCP_FLAG_PSH | TCP_FLAG_ACK, 65535,
                           http_buf, http_len);

            log_debug(http_log, "HTTP response sent\n");
        }

        // FIN → ACK it
        if ((flags & TCP_FLAG_FIN) && !(flags & TCP_FLAG_SYN)) {
            send_tcp_packet(&devices[0], reply_buffer, mac, eth->src,
                           ip->dst_ip, ip->src_ip,
                           dst_port, src_port,
                           their_ack, their_seq + 1,
                           TCP_FLAG_ACK, 65535,
                           NULL, 0);
        }
    }
}
