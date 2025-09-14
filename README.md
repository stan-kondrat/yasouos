# YasouOS

A minimal operating system for virtual machines built from scratch.

## Why?
Just for fun

## Project Goal
Run on AWS EC2 and return "Hello World" on HTTP port

## Features
- Multi-architecture support (ARM64, AMD64, RISC-V)
- VirtIO drivers (virtio-net, virtio-fs)
- Non-POSIX design
- Single core, single process
- stdout only (no stdin)

## Technical Stack
- Language: C23
- Cross-Compilation: GCC toolchains
- Build System: Makefile with multi-architecture support
- Testing Environment: QEMU

## Install Dependencies
```bash
# MacOS
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

## Building

```bash
# All architectures
make                    # Check dependencies, build and test
make build              # Build all architectures
make test               # Test all architectures

# Single Architecture
make ARCH=riscv         # Build and test RISC-V only
make ARCH=arm64 build   # Build ARM64 only
make ARCH=amd64 test    # Test AMD64 only

# Other Commands
make check-deps         # Check build dependencies
make clean              # Remove build files
make help               # Show all available targets
```

## Running

```bash
# Run specific architecture in QEMU (interactive)
make run                # RISC-V default (Ctrl-A X to exit)
make ARCH=riscv run     # RISC-V
make ARCH=arm64 run     # ARM64
make ARCH=amd64 run     # AMD64
```

## Thanks
- Inspired by https://github.com/nuta/operating-system-in-1000-lines
- Claude Code for development assistance

## License
This project is not licensed yet.
