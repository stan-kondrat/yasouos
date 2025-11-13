# Network Drivers

## Supported Devices

- **ARM64**: virtio-net
  - virtio-net: `-device virtio-net-device`

- **RISC-V**: virtio-net, e1000
  - virtio-net: `-device virtio-net-device`
  - e1000: `-device e1000` (PCI)

- **AMD64 (x86_64)**: virtio-net, rtl8139
  - virtio-net: `-device virtio-net-pci`
  - rtl8139: `-device rtl8139` (PCI)

## Applications

Print MAC addresses:
- `app=mac-virtio-net` - VirtIO-Net devices only
- `app=mac-e1000` - E1000 devices only (RISC-V)
- `app=mac-rtl8139` - RTL8139 devices only (AMD64)
- `app=mac-all` - All available network devices
