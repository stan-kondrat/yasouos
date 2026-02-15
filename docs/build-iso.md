# Building a Bootable ISO

## Quick Start

```bash
# Build ISO with default (empty) command line
make ARCH=amd64 build-iso

# Build ISO with custom kernel command line
make ARCH=amd64 build-iso CMDLINE="log=error app=http-hello"

# Run ISO in QEMU
make ARCH=amd64 run-iso
```

Creates `build-iso/yasouos-amd64-<version>.iso`.

## Kernel Command Line

The ISO supports passing kernel command line arguments at build time via the `CMDLINE` variable. The command line is embedded in the kernel binary using a PVH-compatible boot info structure.

Common examples:
- `CMDLINE="log=error"` - Set default log level to ERROR
- `CMDLINE="log=debug app=xxx"` - Enable debug logs and select apps
- `CMDLINE="app=http-hello app=packet-print"` - Run specific applications

See [Kernel Command Line Documentation](kernel-command-line.md) for full syntax.

## Architecture

The ISO uses a self-contained bootloader in `boot_iso.S`:

- **BIOS** loads `boot.bin` to 0x7C00
- **Bootloader** (~120 bytes) prints messages, enables A20, jumps to kernel entry
- **Mode transition** (~180 bytes) handles 16-bit → 32-bit → 64-bit transition
- **Kernel** linked at 0x7D40 with embedded command line via PVH boot info

**Note**: There's a 20-byte alignment gap between Mode transition and Kernel: 0x7C00 (Bootloader) + 0x78 (Bootloader size) + 0xB4 (Mode transition ~180) = 0x7D2C, with 0x14 (20 bytes) padding before kernel at 0x7D40.

### Boot Flow

```
BIOS → Load boot.bin to 0x7C00
     → Bootloader executes (print messages, enable A20)
     → Jump to _start16 (mode transitions)
     → Setup GDT, protected mode, paging
     → Enable PAE and long mode
     → Clear BSS, setup stack
     → Pass PVH boot info (with cmdline) to kernel_main
     → kernel_main() receives cmdline via platform_get_cmdline()
```

## Technical Details

### Binary Structure

`kernel_iso.bin` contains:
- Bootloader code at 0x7C00
- Mode transition code at 0x7C78
- C kernel code and data
- `.cmdline` section with kernel command line
- PVH boot info structure (passes cmdline address)

Padded to 200KB to accommodate BSS, stack, and future growth.

### Files

| File | Description |
|------|-------------|
| `kernel/platform/amd64/boot_iso.S` | Self-contained bootloader + mode transitions + PVH boot info (~340 bytes) |
| `kernel/platform/amd64/kernel_iso.ld` | ISO linker script (includes `.cmdline` section) |
| `build/<arch>/cmdline.S` | Generated at build time with kernel command line |
| `Makefile` | Targets: `build-iso`, `run-iso` (with `CMDLINE` variable) |

## Expected Output

With `CMDLINE="log=error app=http-hello"`:

```
Booting from DVD/CD...
YasouOS ISO Bootloader
Starting kernel...
YasouOS booting...
[AMD64] Exception handlers installed

==================================
       YasouOS v0.1.0
==================================

[INFO][kernel] Hello World from YasouOS!
[INFO][kernel] Architecture: AMD64/x86-64
[INFO][kernel] Kernel command line: log=error app=http-hello

[INFO][devices] Scanning device tree...
[INFO][devices] Found 0004 device(s)
...
```

With empty `CMDLINE` (default): `[INFO][kernel] Kernel command line: (none)`

## Manual QEMU Run

```bash
qemu-system-x86_64 -machine pc -cpu qemu64 -m 128M \
    -cdrom build-iso/yasouos-amd64-*.iso \
    -device e1000,netdev=net0 \
    -netdev user,id=net0,hostfwd=tcp::8088-:80 \
    -nographic --no-reboot
```
