#!/bin/bash
set -e

echo "==================================="
echo "YasouOS AMD64 VirtIO-Net Test"
echo "==================================="

# Detect OS
OS=$(uname -s)
if [ "$OS" != "Darwin" ]; then
    echo "Error: This script is only for macOS"
    exit 1
fi

# Configuration
GUEST_MAC="52:54:00:12:34:56"
GUEST_UDP_PORT="1234"

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
echo -e "\n${YELLOW}Step 1: Building AMD64 kernel...${NC}"
make ARCH=amd64 clean build
echo -e "${GREEN}✓ Build complete${NC}"

# Step 2: Start QEMU with vmnet-bridged (native macOS networking)
echo -e "\n${YELLOW}Step 2: Starting QEMU with virtio-net-pci...${NC}"
echo "Using vmnet-bridged on en0 (requires sudo)"
echo "MAC address: 52:54:00:12:34:56"

echo -e "\n${GREEN}Starting QEMU (Ctrl-A X to exit)...${NC}\n"
sleep 1

sudo qemu-system-x86_64 \
    -machine pc \
    -cpu qemu64 \
    -m 128M \
    -drive file=build/amd64/disk.img,format=raw \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
    -netdev vmnet-bridged,id=net0,ifname=en0 \
    -device virtio-net-pci,netdev=net0,mac=52:54:00:12:34:56 \
    -nographic --no-reboot

echo -e "\n${GREEN}QEMU exited${NC}"
