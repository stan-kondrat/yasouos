# Kernel Command Line Arguments

YasouOS supports command line arguments passed via QEMU's `-append` flag.

**Boot Method Support:**
- **RISC-V**: Kernel boot and disk image boot ✓
- **ARM64**: Kernel boot and disk image boot ✓
- **AMD64**: Kernel boot (PVH protocol) ✓

### RISC-V

```bash
# Boot with kernel binary
qemu-system-riscv64 -machine virt -bios default \
  -kernel build/riscv/kernel.elf \
  -append "lorem ipsum" \
  -nographic --no-reboot

# Boot with disk image
qemu-system-riscv64 -machine virt -bios default \
  -kernel build/riscv/disk.img \
  -append "lorem ipsum" \
  -nographic --no-reboot
```

### ARM64

```bash
# Boot with kernel binary
qemu-system-aarch64 -machine virt -cpu cortex-a53 \
  -kernel build/arm64/kernel.bin \
  -append "lorem ipsum" \
  -nographic --no-reboot

# Boot with disk image
qemu-system-aarch64 -machine virt -cpu cortex-a53 \
  -kernel build/arm64/disk.img \
  -append "lorem ipsum" \
  -nographic --no-reboot
```

## AMD64

```bash
# Boot kernel with command line arguments
qemu-system-x86_64 -machine pc -cpu qemu64 -m 128M \
  -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
  -kernel build/amd64/kernel.elf \
  -append "lorem ipsum" \
  -nographic --no-reboot

# Boot kernel not supported
qemu-system-x86_64 -machine pc -cpu qemu64 -m 128M \
  -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
  -drive "file=build/amd64/disk.img,format=raw" \
  -nographic --no-reboot
```
