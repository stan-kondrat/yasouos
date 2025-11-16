#include "packet_print.h"
#include "../netdev-mac/netdev.h"
#include "../network/net_utils.h"
#include "../network/ethernet/ethernet.h"
#include "../network/arp/arp.h"
#include "../network/ipv4/ipv4.h"
#include "../network/udp/udp.h"
#include "../../common/common.h"

void app_packet_print(void) {
    puts("\n[packet-print] Starting packet-print application...\n");
    device_entry_t devices[1] = {0};
    int device_count = netdev_acquire_all(devices, 1);

    if (device_count < 1) {
        puts("No network devices found\n");
        return;
    }

    resource_print_tag(devices[0].resource);
    puts(" Initializing network device...\n");

    uint8_t mac[6];
    int result = netdev_get_mac(&devices[0], mac);

    if (result == 0) {
        resource_print_tag(devices[0].resource);
        puts(" MAC: ");
        net_print_mac(mac);
        puts("\n");
    }

    resource_print_tag(devices[0].resource);
    puts(" Listening for UDP packets on port ");
    net_print_decimal_u16(PACKET_PRINT_UDP_PORT);
    puts("...\n");

    // Ethernet frame alignment for efficient IPv4 header access
    // Ethernet header is 14 bytes. By offsetting buffer by 2 bytes,
    // the IPv4 header (which starts after Ethernet) will be 4-byte aligned.
    // This improves performance on architectures that require aligned access.
    #define ETH_ALIGNMENT_OFFSET 2
    uint8_t buffer_storage[PACKET_PRINT_BUFFER_SIZE + ETH_ALIGNMENT_OFFSET] __attribute__((aligned(4)));
    uint8_t reply_buffer_storage[PACKET_PRINT_BUFFER_SIZE + ETH_ALIGNMENT_OFFSET] __attribute__((aligned(4)));
    uint8_t *buffer = buffer_storage + ETH_ALIGNMENT_OFFSET;
    uint8_t *reply_buffer = reply_buffer_storage + ETH_ALIGNMENT_OFFSET;
    bool handled_request = false;

    while (!handled_request) {
        size_t received_length = 0;
        result = netdev_receive(&devices[0], buffer, PACKET_PRINT_BUFFER_SIZE, &received_length);


        if (result == 0 && received_length > 0) {
            ethernet_print(buffer, received_length, devices[0].resource, 0);

            if (received_length >= sizeof(eth_hdr_t) + sizeof(ipv4_hdr_t) + sizeof(udp_hdr_t)) {
                const eth_hdr_t *eth = (const eth_hdr_t *)buffer;
                uint16_t eth_type = ntohs(eth->type);

                if (eth_type == ETH_P_IP) {
                    const ipv4_hdr_t *ip = (const ipv4_hdr_t *)(buffer + sizeof(eth_hdr_t));

                    if (ip->protocol == IPPROTO_UDP) {
                        uint32_t dst_ip = ntohl(ip->dst_ip);

                        if (dst_ip == PACKET_PRINT_IP_ADDR) {
                            const udp_hdr_t *udp = (const udp_hdr_t *)(buffer + sizeof(eth_hdr_t) + sizeof(ipv4_hdr_t));
                            uint16_t dst_port = ntohs(udp->dst_port);

                            if (dst_port == PACKET_PRINT_UDP_PORT) {
                                // Extract payload
                                const uint8_t *payload = buffer + sizeof(eth_hdr_t) + sizeof(ipv4_hdr_t) + sizeof(udp_hdr_t);
                                size_t payload_len = ntohs(udp->length) - sizeof(udp_hdr_t);

                                resource_print_tag(devices[0].resource);
                                puts(" Received UDP payload: ");
                                for (size_t i = 0; i < payload_len && i < PACKET_PRINT_MAX_PAYLOAD_DISPLAY; i++) {
                                    char c = payload[i];
                                    if (c >= 32 && c <= 126) {
                                        putchar(c);
                                    } else {
                                        putchar('.');
                                    }
                                }
                                puts("\n");

                                // Check if payload starts with "ping-"
                                if (payload_len >= 6 &&
                                    payload[0] == 'p' && payload[1] == 'i' &&
                                    payload[2] == 'n' && payload[3] == 'g' &&
                                    payload[4] == '-') {

                                    // Parse the number after "ping-"
                                    int num = 0;
                                    for (size_t i = 5; i < payload_len && payload[i] >= '0' && payload[i] <= '9'; i++) {
                                        num = num * 10 + (payload[i] - '0');
                                    }

                                    // Build response: "pong-" + (num + 1)
                                    uint8_t reply_payload_buf[64];
                                    size_t response_len = 0;

                                    reply_payload_buf[response_len++] = 'p';
                                    reply_payload_buf[response_len++] = 'o';
                                    reply_payload_buf[response_len++] = 'n';
                                    reply_payload_buf[response_len++] = 'g';
                                    reply_payload_buf[response_len++] = '-';

                                    // Convert (num + 1) to ASCII digits
                                    int response_num = num + 1;
                                    char num_str[16];
                                    int num_len = 0;
                                    if (response_num == 0) {
                                        num_str[num_len++] = '0';
                                    } else {
                                        int temp = response_num;
                                        while (temp > 0) {
                                            num_str[num_len++] = '0' + (temp % 10);
                                            temp /= 10;
                                        }
                                        // Reverse the digits
                                        for (int i = 0; i < num_len / 2; i++) {
                                            char tmp = num_str[i];
                                            num_str[i] = num_str[num_len - 1 - i];
                                            num_str[num_len - 1 - i] = tmp;
                                        }
                                    }
                                    for (int i = 0; i < num_len; i++) {
                                        reply_payload_buf[response_len++] = num_str[i];
                                    }

                                    // Build UDP echo reply
                                    eth_hdr_t *reply_eth = (eth_hdr_t *)reply_buffer;
                                    for (int i = 0; i < 6; i++) {
                                        reply_eth->dst[i] = eth->src[i];
                                        reply_eth->src[i] = mac[i];
                                    }
                                    reply_eth->type = htons(ETH_P_IP);

                                    // Build IPv4 header
                                    ipv4_hdr_t *reply_ip = (ipv4_hdr_t *)(reply_buffer + sizeof(eth_hdr_t));
                                    ipv4_build_header(reply_ip, ip->dst_ip, ip->src_ip, IPPROTO_UDP, sizeof(udp_hdr_t) + response_len, 64);

                                    // Build UDP header
                                    udp_hdr_t *reply_udp = (udp_hdr_t *)(reply_buffer + sizeof(eth_hdr_t) + sizeof(ipv4_hdr_t));
                                    uint16_t src_port = ntohs(udp->dst_port);
                                    uint16_t dst_port = ntohs(udp->src_port);
                                    udp_build_header(reply_udp, src_port, dst_port, response_len);

                                    // Copy response payload
                                    uint8_t *reply_payload = reply_buffer + sizeof(eth_hdr_t) + sizeof(ipv4_hdr_t) + sizeof(udp_hdr_t);
                                    for (size_t i = 0; i < response_len; i++) {
                                        reply_payload[i] = reply_payload_buf[i];
                                    }

                                    size_t total_len = sizeof(eth_hdr_t) + sizeof(ipv4_hdr_t) + sizeof(udp_hdr_t) + response_len;

                                    result = netdev_transmit(&devices[0], reply_buffer, total_len);
                                    if (result == 0) {
                                        resource_print_tag(devices[0].resource);
                                        puts(" Sent UDP echo reply\n");
                                        handled_request = true;
                                    }
                                }
                            }
                        }
                    }
                } else if (eth_type == ETH_P_ARP) {
                    // Handle ARP requests
                    const arp_hdr_t *arp_req = (const arp_hdr_t *)(buffer + sizeof(eth_hdr_t));
                    uint16_t opcode = ntohs(arp_req->opcode);
                    uint32_t target_ip = ntohl(arp_req->target_ip);

                    if (opcode == ARP_OP_REQUEST && target_ip == PACKET_PRINT_IP_ADDR) {
                        uint32_t sender_ip = ntohl(arp_req->sender_ip);

                        arp_build_reply(reply_buffer,
                                       mac,
                                       PACKET_PRINT_IP_ADDR,
                                       arp_req->sender_mac,
                                       sender_ip);

                        result = netdev_transmit(&devices[0], reply_buffer, ARP_PACKET_SIZE);
                        if (result == 0) {
                            resource_print_tag(devices[0].resource);
                            puts(" Sent ARP reply\n");
                        }
                    }
                }
            }
        }
    }
}

