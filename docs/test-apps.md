# Test Applications

YasouOS includes several test applications for verifying OS functionality.

## illegal-instruction

Tests exception handling by executing illegal instructions.

Triggers architecture-specific undefined instructions (UDF on ARM64, UD2 on x86, invalid opcode on RISC-V) to verify the OS properly catches and handles CPU exceptions.

## random

Tests hardware random number generation with software fallback.

Acquires virtio-rng devices and reads random bytes from hardware RNG. Falls back to xorshift64 PRNG when hardware unavailable. Supports multiple RNG devices with round-robin selection.

## netdev-mac

Tests network device initialization and MAC address retrieval.

Includes multiple variants:
- `mac_all` - Acquires all network devices and displays MAC addresses
- `mac_virtio_net` - Tests VirtIO network devices
- `mac_e1000` - Tests Intel E1000 network devices
- `mac_rtl8139` - Tests Realtek RTL8139 network devices

## arp-broadcast

Tests network packet transmission and reception using ARP protocol.

Acquires three network devices, builds ARP broadcast packet (who-has request), transmits from first device, and receives on remaining devices. Validates broadcast functionality across multiple network interfaces.
