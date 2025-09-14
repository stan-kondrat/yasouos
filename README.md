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

## Install Dependencies

```bash
# macOS
brew install gcc qemu

# Ubuntu
apt install gcc qemu
```

## Building
```bash
make # build and test all
make build
make build ARCH=riscv
make test
make test ARCH=riscv
```

## Running
```bash
# Run in QEMU with specific architecture
make run # ARCH=riscv default
make run ARCH=riscv
make run ARCH=arm64
make run ARCH=amd64
```

Open http://localhost:8080 to check "Hello World"

## Deploy to EC2
```bash
# Build for target architecture
make build ARCH=arm64

# Deploy to EC2 instance
# (Instructions will be added)
```

## Technical Stack
- Language: C23
- Compiler: GCC 15
- Build system: Makefile
- Testing environment: QEMU

## License
This project is not licensed yet.
