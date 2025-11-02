#!/bin/bash
set -e

echo "==================================="
echo "YasouOS RISC-V VirtIO-Net Test"
echo "==================================="

# Detect OS
OS=$(uname -s)
if [ "$OS" != "Darwin" ]; then
    echo "Error: This script is only for macOS"
    exit 1
fi

# Configuration
GUEST_MAC="52:54:00:12:34:56"

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

# Step 2: Start QEMU with vmnet-bridged (native macOS networking)
echo -e "\n${YELLOW}Step 2: Starting QEMU with virtio-net-device...${NC}"
echo "Using vmnet-bridged on en0 (requires sudo)"
echo "MAC address: 52:54:00:12:34:56"
echo -e "${YELLOW}Note: RISC-V device tree stub currently returns 0 devices${NC}"

echo -e "\n${GREEN}Starting QEMU (Ctrl-A X to exit)...${NC}\n"
sleep 1

sudo qemu-system-riscv64 \
    -machine virt \
    -bios default \
    -m 128M \
    -kernel build/riscv/kernel.elf \
    -netdev vmnet-bridged,id=net0,ifname=en0 \
    -device virtio-net-device,netdev=net0,mac=52:54:00:12:34:56 \
    -nographic --no-reboot

echo -e "\n${GREEN}QEMU exited${NC}"
