#include "arp_broadcast.h"
#include "../../common/common.h"
#include "../netdev-mac/netdev.h"

#define MAX_DEVICES 3
#define ARP_PACKET_SIZE 42
#define BROADCAST_MAC {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}

// EtherType constants
#define ETH_P_ARP   0x0806
#define ETH_P_IP    0x0800

// ARP constants
#define ARP_HW_ETHER    1
#define ARP_OP_REQUEST  1
#define ARP_OP_REPLY    2

// Byte order conversions
static inline uint16_t htons(uint16_t x) {
    return (x >> 8) | (x << 8);
}

static inline uint16_t ntohs(uint16_t x) {
    return (x >> 8) | (x << 8);
}

static inline uint32_t htonl(uint32_t x) {
    return ((x >> 24) & 0x000000FF) |
           ((x >> 8)  & 0x0000FF00) |
           ((x << 8)  & 0x00FF0000) |
           ((x << 24) & 0xFF000000);
}

static inline uint32_t ntohl(uint32_t x) {
    return ((x >> 24) & 0x000000FF) |
           ((x >> 8)  & 0x0000FF00) |
           ((x << 8)  & 0x00FF0000) |
           ((x << 24) & 0xFF000000);
}

// Ethernet header
typedef struct {
    uint8_t  dst[6];
    uint8_t  src[6];
    uint16_t type;
} __attribute__((packed)) eth_hdr_t;

// ARP packet
typedef struct {
    uint16_t hw_type;
    uint16_t proto_type;
    uint8_t  hw_len;
    uint8_t  proto_len;
    uint16_t opcode;
    uint8_t  sender_mac[6];
    uint32_t sender_ip;
    uint8_t  target_mac[6];
    uint32_t target_ip;
} __attribute__((packed)) arp_hdr_t;

static void build_arp_packet(uint8_t *packet,
                            const uint8_t sender_mac[6],
                            uint32_t sender_ip,
                            uint32_t target_ip) {
    eth_hdr_t *eth = (eth_hdr_t *)packet;
    arp_hdr_t *arp = (arp_hdr_t *)(packet + sizeof(eth_hdr_t));

    // Ethernet header
    uint8_t broadcast[6] = BROADCAST_MAC;
    for (int i = 0; i < 6; i++) {
        eth->dst[i] = broadcast[i];
        eth->src[i] = sender_mac[i];
    }
    eth->type = htons(ETH_P_ARP);

    // ARP header
    arp->hw_type = htons(ARP_HW_ETHER);
    arp->proto_type = htons(ETH_P_IP);
    arp->hw_len = 6;
    arp->proto_len = 4;
    arp->opcode = htons(ARP_OP_REQUEST);

    // ARP body
    for (int i = 0; i < 6; i++) {
        arp->sender_mac[i] = sender_mac[i];
        arp->target_mac[i] = 0x00;
    }
    arp->sender_ip = htonl(sender_ip);
    arp->target_ip = htonl(target_ip);
}

static void print_mac(const uint8_t mac[6]) {
    for (int i = 0; i < 6; i++) {
        put_hex8(mac[i]);
        if (i < 5) {
            puts(":");
        }
    }
}

static void print_decimal(uint8_t value) {
    if (value >= 100) {
        putchar('0' + (value / 100));
        value %= 100;
        putchar('0' + (value / 10));
        putchar('0' + (value % 10));
    } else if (value >= 10) {
        putchar('0' + (value / 10));
        putchar('0' + (value % 10));
    } else {
        putchar('0' + value);
    }
}

static void print_ip(uint32_t ip) {
    print_decimal((ip >> 24) & 0xFF);
    puts(".");
    print_decimal((ip >> 16) & 0xFF);
    puts(".");
    print_decimal((ip >> 8) & 0xFF);
    puts(".");
    print_decimal(ip & 0xFF);
}

static void parse_arp_packet(const uint8_t *packet, size_t length) {
    if (length < ARP_PACKET_SIZE) {
        puts("Packet too small\n");
        return;
    }

    const eth_hdr_t *eth = (const eth_hdr_t *)packet;
    const arp_hdr_t *arp = (const arp_hdr_t *)(packet + sizeof(eth_hdr_t));

    puts("\tDST=");
    print_mac(eth->dst);
    puts(" SRC=");
    print_mac(eth->src);
    puts(" TYPE=0x");
    put_hex16(ntohs(eth->type));
    puts(" (ARP)\n");

    uint16_t opcode = ntohs(arp->opcode);
    if (opcode == ARP_OP_REQUEST) {
        puts("\tARP who-has ");
        print_ip(ntohl(arp->target_ip));
        puts(" tell ");
        print_ip(ntohl(arp->sender_ip));
        puts("\n");
    } else if (opcode == ARP_OP_REPLY) {
        puts("\tARP reply from ");
        print_ip(ntohl(arp->sender_ip));
        puts("\n");
    }
}

void app_arp_broadcast(void) {
    device_entry_t devices[MAX_DEVICES] = {0};
    int device_count = netdev_acquire_all(devices, MAX_DEVICES);

    if (device_count < 3) {
        puts("Error: Need at least 3 network devices for ARP broadcast test\n");
        return;
    }

    // Get MAC addresses
    uint8_t sender_mac[6];
    uint8_t receiver1_mac[6];
    uint8_t receiver2_mac[6];

    int result_sender = netdev_get_mac(&devices[0], sender_mac);
    int result_receiver1 = netdev_get_mac(&devices[1], receiver1_mac);
    int result_receiver2 = netdev_get_mac(&devices[2], receiver2_mac);

    if (result_sender != 0 || result_receiver1 != 0 || result_receiver2 != 0) {
        puts("Error: Failed to get MAC addresses\n");
        return;
    }

    // Print device information
    resource_print_tag(devices[0].resource);
    puts(" Initializing...\n");
    resource_print_tag(devices[0].resource);
    puts(" MAC: ");
    print_mac(sender_mac);
    puts("\n\n");

    resource_print_tag(devices[1].resource);
    puts(" Initializing...\n");
    resource_print_tag(devices[1].resource);
    puts(" MAC: ");
    print_mac(receiver1_mac);
    puts("\n\n");

    resource_print_tag(devices[2].resource);
    puts(" Initializing...\n");
    resource_print_tag(devices[2].resource);
    puts(" MAC: ");
    print_mac(receiver2_mac);
    puts("\n\n");

    // Build ARP packet
    uint8_t arp_packet[64] = {0};
    uint32_t sender_ip = 0x0A000201;    // 10.0.2.1
    uint32_t target_ip = 0x0A00020F;    // 10.0.2.15

    build_arp_packet(arp_packet, sender_mac, sender_ip, target_ip);

    // Print transmission info
    resource_print_tag(devices[0].resource);
    puts(" TX: Building ARP broadcast\n");
    parse_arp_packet(arp_packet, ARP_PACKET_SIZE);
    resource_print_tag(devices[0].resource);
    puts(" TX: Length=");
    print_decimal(ARP_PACKET_SIZE);
    puts(" bytes\n");

    // Transmit packet using real network driver
    int tx_result = netdev_transmit(&devices[0], arp_packet, ARP_PACKET_SIZE);
    if (tx_result == 0) {
        resource_print_tag(devices[0].resource);
        puts(" TX: Packet sent successfully\n\n");
    } else {
        resource_print_tag(devices[0].resource);
        puts(" TX: Failed to send packet\n\n");
    }

    // Receive on second device
    resource_print_tag(devices[1].resource);
    puts(" RX: Waiting for packet...\n");

    uint8_t rx_buffer1[64];
    size_t rx_length1 = 0;
    int rx_result1 = netdev_receive(&devices[1], rx_buffer1, sizeof(rx_buffer1), &rx_length1);

    if (rx_result1 == 0) {
        resource_print_tag(devices[1].resource);
        puts(" RX: Packet received (");
        print_decimal(rx_length1);
        puts(" bytes)\n");
        parse_arp_packet(rx_buffer1, rx_length1);
    } else {
        resource_print_tag(devices[1].resource);
        puts(" RX: No packet received\n");
    }

    puts("\n");

    // Receive on third device
    resource_print_tag(devices[2].resource);
    puts(" RX: Waiting for packet...\n");

    uint8_t rx_buffer2[64];
    size_t rx_length2 = 0;
    int rx_result2 = netdev_receive(&devices[2], rx_buffer2, sizeof(rx_buffer2), &rx_length2);

    if (rx_result2 == 0) {
        resource_print_tag(devices[2].resource);
        puts(" RX: Packet received (");
        print_decimal(rx_length2);
        puts(" bytes)\n");
        parse_arp_packet(rx_buffer2, rx_length2);
    } else {
        resource_print_tag(devices[2].resource);
        puts(" RX: No packet received\n");
    }
}
