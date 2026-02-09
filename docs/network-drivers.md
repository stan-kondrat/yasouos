# Network Drivers

## Supported Devices

- **ARM64**: virtio-net, e1000, rtl8139
  - virtio-net: `-device virtio-net-device`
  - e1000: `-device e1000`
  - rtl8139: `-device rtl8139`

- **RISC-V**: virtio-net, e1000, rtl8139
  - virtio-net: `-device virtio-net-device`
  - e1000: `-device e1000`
  - rtl8139: `-device rtl8139`

- **AMD64 (x86_64)**: virtio-net, e1000, rtl8139
  - virtio-net: `-device virtio-net-pci`
  - e1000: `-device e1000`
  - rtl8139: `-device rtl8139`

## Applications

Print MAC addresses:
- `app=mac-virtio-net` - VirtIO-Net devices only
- `app=mac-e1000` - E1000 devices only
- `app=mac-rtl8139` - RTL8139 devices only
- `app=mac-all` - All available network devices

HTTP server:
- `app=http-hello` - HTTP server on port 80, responds with "Hello, \<client-ip\>"

Packet inspection:
- `app=packet-print` - Print received network packets (Ethernet, ARP, IPv4, TCP, UDP, ICMP)
