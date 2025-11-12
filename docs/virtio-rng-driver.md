# VirtIO RNG Driver

## VirtIO Transport Methods

| Architecture | Transport | Method | QEMU Machine Type | Device Type |
|--------------|-----------|--------|-------------------|-------------|
| ARM64 | MMIO | FDT | `-M virt` | `virtio-rng-device` |
| RISC-V | MMIO | FDT | `-M virt` | `virtio-rng-device` |
| AMD64 | PCI | Legacy I/O ports | `-M pc` | `virtio-rng-pci` |
| AMD64 | PCI | MMCONFIG/ECAM | `-M q35` | `virtio-rng-pci` |

**Important Notes:**
- **RISC-V/ARM64** use VirtIO MMIO transport (`virtio-rng-device`) because:
  - QEMU virt machine doesn't allocate MMIO addresses for PCI BARs
  - RISC-V doesn't support I/O port addressing
  - Devices are enumerated via FDT (Flattened Device Tree)
- **AMD64** uses VirtIO PCI transport (`virtio-rng-pci`) with I/O port access

## QEMU Examples

### ARM64 (VirtIO MMIO, `-M virt`)

```sh
qemu-system-aarch64 -machine virt -cpu cortex-a53 \
  -kernel build/arm64/kernel.bin \
  -nographic --no-reboot \
  -object rng-random,id=rng0,filename=/dev/urandom \
  -device virtio-rng-device,rng=rng0 \
  -object rng-random,id=rng1,filename=/dev/random \
  -device virtio-rng-device,rng=rng1 \
  -append "app=random-hardware app=random-hardware"
```

### RISC-V (VirtIO MMIO, `-M virt`)

```sh
qemu-system-riscv64 -machine virt -bios default \
  -kernel build/riscv/kernel.elf \
  -nographic --no-reboot \
  -object rng-random,id=rng0,filename=/dev/urandom \
  -device virtio-rng-device,rng=rng0 \
  -object rng-random,id=rng1,filename=/dev/random \
  -device virtio-rng-device,rng=rng1 \
  -append "app=random-hardware app=random-hardware"
```

### AMD64 (VirtIO PCI with Legacy I/O ports, `-M pc`)

```sh
qemu-system-x86_64 -machine pc -cpu qemu64 -m 128M \
  -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
  -kernel build/amd64/kernel.elf \
  -nographic --no-reboot \
  -object rng-random,id=rng0,filename=/dev/urandom \
  -device virtio-rng-pci,rng=rng0 \
  -object rng-random,id=rng1,filename=/dev/random \
  -device virtio-rng-pci,rng=rng1 \
  -append "app=random-hardware app=random-hardware"
```

### AMD64 (VirtIO PCI with MMCONFIG/ECAM, `-M q35`)

```sh
qemu-system-x86_64 -machine q35 -cpu qemu64 -m 128M \
  -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
  -kernel build/amd64/kernel.elf \
  -nographic --no-reboot \
  -object rng-random,id=rng0,filename=/dev/urandom \
  -device virtio-rng-pci,rng=rng0 \
  -object rng-random,id=rng1,filename=/dev/random \
  -device virtio-rng-pci,rng=rng1 \
  -append "app=random-hardware app=random-hardware"
```