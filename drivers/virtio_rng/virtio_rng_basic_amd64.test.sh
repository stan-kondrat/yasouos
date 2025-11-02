#!/bin/bash
set -e

echo "==================================="
echo "YasouOS AMD64 VirtIO-RNG Test"
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
echo -e "\n${YELLOW}Step 1: Building AMD64 kernel...${NC}"
make ARCH=amd64 clean build
echo -e "${GREEN}✓ Build complete${NC}"

# Step 2: Start QEMU with virtio-rng device
echo -e "\n${YELLOW}Step 2: Starting QEMU with virtio-rng-pci...${NC}"
echo "The kernel should detect and initialize the VirtIO RNG device"

echo -e "\n${GREEN}Starting QEMU (Ctrl-A X to exit)...${NC}\n"
sleep 1


qemu-system-x86_64 \
    -machine pc \
    -cpu qemu64 \
    -m 128M \
    -drive file=build/amd64/disk.img,format=raw \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
    -device virtio-rng-pci \
    -nographic --no-reboot


