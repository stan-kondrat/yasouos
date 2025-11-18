#pragma once

#include "../network/ipv4/ipv4.h"

#define PACKET_PRINT_BUFFER_SIZE 2048
#define PACKET_PRINT_IP_ADDR IPV4(10, 0, 2, 15)
#define PACKET_PRINT_UDP_PORT 5000
#define PACKET_PRINT_IPV4_PORT 8080
#define PACKET_PRINT_MAX_PAYLOAD_DISPLAY 64

void app_packet_print(void);
