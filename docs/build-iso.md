# Building a Bootable ISO with Limine

Create a bootable ISO for deploying YasouOS on real hardware or cloud providers (e.g., Vultr).
Uses [Limine](https://codeberg.org/Limine/Limine) bootloader with multiboot1 protocol, which passes kernel command line arguments through the existing `platform_get_cmdline()` path.


## Configuration

Create `limine.conf` with your desired kernel arguments:

```
timeout: 0

/YasouOS
    protocol: multiboot1
    path: boot():/kernel.elf
    cmdline: app=http-hello log=warn
```

### Kernel argument examples

```
# HTTP server with minimal logging
cmdline: app=http-hello log=warn

# HTTP server with verbose network logging
cmdline: app=http-hello log=warn log.http-hello=debug

# Packet printer with full debug
cmdline: log=debug app=packet-print

# ARP broadcast
cmdline: log=debug app=arp-broadcast
```

See [kernel-command-line.md](kernel-command-line.md) and [logging.md](logging.md) for all options.

## Build the ISO

### Step 1: Build the kernel

```bash
make build ARCH=amd64
```

### Step 2: Assemble the ISO root

```bash
mkdir -p build-iso
cp build/amd64/kernel.elf build-iso/
cp /usr/share/limine/limine-bios.sys build-iso/
cp /usr/share/limine/limine-bios-cd.bin build-iso/

cat > build-iso/limine.conf << 'EOF'
timeout: 0

/YasouOS
    protocol: multiboot1
    path: boot():/kernel.elf
    cmdline: app=http-hello log=warn
EOF
```

### Step 3: Create the ISO

```bash
ARCH=amd64
VERSION=$(git describe --tags 2>/dev/null || git rev-parse --short HEAD)
ISO_NAME="build-iso/yasouos-${ARCH}-${VERSION}.iso"

xorriso -as mkisofs -R -r -J \
    -b limine-bios-cd.bin \
    -no-emul-boot -boot-load-size 4 -boot-info-table \
    build-iso/ -o "$ISO_NAME"
```

### Step 4: Install Limine BIOS boot sectors

```bash
limine bios-install "$ISO_NAME"
```

## Test locally with QEMU

```bash
qemu-system-x86_64 -machine pc -cpu qemu64 -m 128M \
    -cdrom "$ISO_NAME" \
    -device e1000,netdev=net0 \
    -netdev user,id=net0,hostfwd=tcp::8088-:80 \
    -nographic --no-reboot
```

## Changing kernel arguments

Edit `cmdline` in `build-iso/limine.conf` and repeat steps 3-4. No kernel rebuild needed.
