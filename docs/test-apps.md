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

## http-hello

HTTP server that responds with "Hello, \<client-ip\>" to every request on port 80.

Stateless TCP handler — no connection tracking. Each packet is handled independently:
- SYN → SYN+ACK
- Data (HTTP GET) → HTTP response + FIN
- FIN → ACK

Responds to any IP address (no hardcoded IP). ARP replies are sent for any target IP.

### Running

```bash
# Build first
make ARCH=arm64 build
# ARM64 with virtio-net
qemu-system-aarch64 -machine virt -cpu cortex-a53 \
  -kernel build/arm64/kernel.bin \
  -append "app=http-hello" \
  -device virtio-net-device,netdev=net0,mac=52:54:00:12:34:56 \
  -netdev user,id=net0,hostfwd=tcp::8088-:80 \
  -nographic --no-reboot
```

Then from another terminal:

```bash
curl http://127.0.0.1:8088/
# Hello, 10.0.2.2
```

## arp-broadcast

Tests network packet transmission and reception using ARP protocol.

Acquires three network devices, builds ARP broadcast packet (who-has request), transmits from first device, and receives on remaining devices. Validates broadcast functionality across multiple network interfaces.
