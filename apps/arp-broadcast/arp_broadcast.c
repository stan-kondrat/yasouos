#include "arp_broadcast.h"
#include "../../common/common.h"
#include "../netdev-mac/netdev.h"
#include "../network/arp/arp.h"
#include "../network/net_utils.h"

#define MAX_DEVICES 3

void app_arp_broadcast(void) {
    device_entry_t devices[MAX_DEVICES] = {0};
    int device_count = netdev_acquire_all(devices, MAX_DEVICES);

    if (device_count < 3) {
        puts("Error: Need at least 3 network devices for ARP broadcast test\n");
        return;
    }

    uint8_t mac_addresses[MAX_DEVICES][6];

    for (int i = 0; i < device_count; i++) {
        resource_print_tag(devices[i].resource);
        puts(" Initializing...\n");

        int result = netdev_get_mac(&devices[i], mac_addresses[i]);
        if (result != 0) {
            resource_print_tag(devices[i].resource);
            puts(" Error: Failed to get MAC address\n");
            return;
        }

        resource_print_tag(devices[i].resource);
        puts(" MAC: ");
        net_print_mac(mac_addresses[i]);
        puts("\n\n");
    }

    uint8_t arp_packet[64] = {0};
    uint32_t sender_ip = 0x0A000201;
    uint32_t target_ip = 0x0A00020F;

    arp_build_request(arp_packet, mac_addresses[0], sender_ip, target_ip);

    resource_print_tag(devices[0].resource);
    puts(" TX: Building ARP broadcast\n");

    const arp_hdr_t *tx_arp = arp_parse(arp_packet, ARP_PACKET_SIZE, NULL);
    if (tx_arp) {
        arp_print(tx_arp, 0);
    }

    resource_print_tag(devices[0].resource);
    puts(" TX: Length=");
    net_print_decimal_u8(ARP_PACKET_SIZE);
    puts(" bytes\n");

    int tx_result = netdev_transmit(&devices[0], arp_packet, ARP_PACKET_SIZE);
    if (tx_result == 0) {
        resource_print_tag(devices[0].resource);
        puts(" TX: Packet sent successfully\n\n");
    } else {
        resource_print_tag(devices[0].resource);
        puts(" TX: Failed to send packet\n\n");
        return;
    }

    for (int i = 1; i < device_count; i++) {
        resource_print_tag(devices[i].resource);
        puts(" RX: Waiting for packet...\n");

        uint8_t rx_buffer[64];
        size_t rx_length = 0;
        int rx_result = netdev_receive(&devices[i], rx_buffer, sizeof(rx_buffer), &rx_length);

        if (rx_result == 0) {
            resource_print_tag(devices[i].resource);
            puts(" RX: Packet received (");
            net_print_decimal_u8(rx_length);
            puts(" bytes)\n");

            const arp_hdr_t *rx_arp = arp_parse(rx_buffer, rx_length, NULL);
            if (rx_arp) {
                arp_print(rx_arp, 0);
            }
        } else {
            resource_print_tag(devices[i].resource);
            puts(" RX: No packet received\n");
        }

        puts("\n");
    }
}
