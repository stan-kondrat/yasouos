#!/bin/bash
set -e

echo "==================================="
echo "YasouOS RISC-V VirtIO-RNG Test"
echo "==================================="

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Cleanup function
cleanup() {
    echo -e "\n${YELLOW}Cleaning up...${NC}"

    # Kill QEMU if running
    if [ ! -z "$QEMU_PID" ]; then
        kill $QEMU_PID 2>/dev/null || true
        wait $QEMU_PID 2>/dev/null || true
    fi

    echo -e "${GREEN}Cleanup complete${NC}"
}

trap cleanup EXIT

# Step 1: Build project
echo -e "\n${YELLOW}Step 1: Building RISC-V kernel...${NC}"
make ARCH=riscv clean build
echo -e "${GREEN}✓ Build complete${NC}"

# Step 2: Start QEMU with virtio-rng device
echo -e "\n${YELLOW}Step 2: Starting QEMU with virtio-rng-device...${NC}"
echo "The kernel should detect and initialize the VirtIO RNG device"
echo -e "${YELLOW}Note: RISC-V device tree stub currently returns 0 devices${NC}"

echo -e "\n${GREEN}Starting QEMU (Ctrl-A X to exit)...${NC}\n"
sleep 1


qemu-system-riscv64 \
    -machine virt \
    -bios default \
    -m 128M \
    -kernel build/riscv/kernel.elf \
    -device virtio-rng-device \
    -nographic --no-reboot
