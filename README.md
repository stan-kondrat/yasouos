# YasouOS

A minimal operating system for virtual machines built from scratch.

## Why?
Just for fun

## Project Goal
Run on AWS EC2 and return "Hello World" on HTTP port

## Features
- Multi-architecture support (ARM64, AMD64, RISC-V)
- VirtIO drivers
- Non-POSIX design
- Single core, single process
- stdout only (no stdin)

## Supported Drivers

YasouOS includes support for the following PCI devices:

| Driver | Vendor ID | Device ID | Name | Status |
|--------|-----------|-----------|------|--------|
| **virtio-net** | 0x1af4 | 0x1000 | VirtIO Network| ✅ Working |
| **virtio-blk** | 0x1af4 | 0x1001 | VirtIO Block Device | 🚧 Stub |
| **virtio-rng** | 0x1af4 | 0x1005 | VirtIO RNG (Entropy Source) | 🚧 Stub |


## Technical Stack
- Language: C23
- Cross-Compilation: GCC 15 toolchains
- Build System: Makefile with multi-architecture support
- Testing Environment: QEMU 10

## Install Dependencies
```bash
# MacOS
brew install gcc@15
brew install riscv64-elf-gcc        # RISC-V 64-bit
brew install aarch64-elf-gcc        # ARM64
brew install x86_64-elf-gcc         # AMD64

# QEMU for testing
brew install qemu                   # All architectures

# Ubuntu equivalents:
# apt install gcc-riscv64-unknown-elf
# apt install gcc-aarch64-linux-gnu
# apt install gcc-multilib  # for x86_64
```

## Build System

The project uses a simplified Makefile that supports all three architectures with template-based target generation:

```bash
make                    # Build and test all architectures (default)

# Build all architectures
make build              # Builds kernels and disk images for all archs

# Build specific architecture
make ARCH=riscv build   # RISC-V only
make ARCH=arm64 build   # ARM64 only
make ARCH=amd64 build   # AMD64 only

# Testing
make test               # Test all architectures
make ARCH=riscv test    # Test RISC-V only
make ARCH=arm64 test    # Test ARM64 only
make ARCH=amd64 test    # Test AMD64 only

# Utility commands
make check-deps         # Check dependencies for all architectures
make clean              # Remove all build files
make help               # Show all available targets
```

## Running in QEMU

```bash
# Run kernel directly (requires ARCH specification)
make ARCH=riscv run     # RISC-V (Ctrl-A X to exit)
make ARCH=arm64 run     # ARM64
make ARCH=amd64 run     # AMD64

# Run from disk image (bootable)
make ARCH=amd64 run-img # AMD64 disk image (with bootloader)
```

## Architecture Support

The build system automatically detects available cross-compilers:

- **RISC-V**: `riscv64-elf-gcc`, `riscv64-linux-gnu-gcc`, or `riscv64-unknown-elf-gcc`
- **ARM64**: `aarch64-elf-gcc` or `aarch64-linux-gnu-gcc`
- **AMD64**: `x86_64-elf-gcc`, `x86_64-linux-gnu-gcc`, or native `gcc` (on x86_64)

## Thanks
- Inspired by https://github.com/nuta/operating-system-in-1000-lines
- Claude Code for development assistance

## License
This project is not licensed yet.

