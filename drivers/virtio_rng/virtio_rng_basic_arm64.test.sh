#!/bin/bash
set -e

echo "==================================="
echo "YasouOS ARM64 VirtIO-RNG Test"
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
echo -e "\n${YELLOW}Step 1: Building ARM64 kernel...${NC}"
make ARCH=arm64 clean build
echo -e "${GREEN}✓ Build complete${NC}"

# Step 2: Start QEMU with virtio-rng device
echo -e "\n${YELLOW}Step 2: Starting QEMU with virtio-rng-device...${NC}"
echo "The kernel should detect and initialize the VirtIO RNG device"
echo -e "${YELLOW}Note: ARM64 device tree stub currently returns 0 devices${NC}"

echo -e "\n${GREEN}Starting QEMU (Ctrl-A X to exit)...${NC}\n"
sleep 1


qemu-system-aarch64 \
    -machine virt \
    -cpu cortex-a53 \
    -m 128M \
    -semihosting-config enable=on,target=native \
    -kernel build/arm64/kernel.elf \
    -device virtio-rng-device \
    -nographic --no-reboot
