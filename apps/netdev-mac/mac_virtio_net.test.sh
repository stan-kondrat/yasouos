#!/bin/bash


# Test virtio-net MAC address reading
# Usage: ./apps/netdev-mac/mac_virtio_net.test.sh [-v] [riscv|arm64|amd64] [kernel|image]
#
# Examples:
#   ./apps/netdev-mac/mac_virtio_net.test.sh              # Run all architectures
#   ./apps/netdev-mac/mac_virtio_net.test.sh -v           # Run all with verbose output
#   ./apps/netdev-mac/mac_virtio_net.test.sh arm64        # Run ARM64 only
#   ./apps/netdev-mac/mac_virtio_net.test.sh -v riscv     # Run RISC-V with verbose output
#   ./apps/netdev-mac/mac_virtio_net.test.sh -v amd64 image


SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$PROJECT_ROOT/tests/common.sh"

init_test_matrix "$@" "Testing VirtIO network driver"

for arch in $TEST_MATRIX_ARCH; do
    for boot_type in $TEST_MATRIX_BOOT_TYPE; do
        test_section "virtio-net single device ($arch $boot_type)"

        # AMD64 raw image doesn't support command line args (no bootloader yet)
        if [ "$arch" = "amd64" ] && [ "$boot_type" = "image" ]; then
            skip_test_case "AMD64 raw image doesn't support command line args"
            continue
        fi

        qemu_cmd=$(get_full_qemu_cmd "$arch" "$boot_type")

        if [ "$arch" = "amd64" ]; then
            device="virtio-net-pci"
        else
            device="virtio-net-device"
        fi

        # single device
        qemu_args=(
            -append "'log=debug app=mac-virtio-net'"
            -device "$device,netdev=net0,mac=52:54:00:12:34:56"
            -netdev hubport,id=net0,hubid=0
        )
        output=$(run_test_case "$qemu_cmd ${qemu_args[*]}")
        assert_count "$output" "MAC: 52:54:00:12:34:56" 1 "MAC address found"


        # two devices
        test_section "virtio-net two devices ($arch $boot_type)"

        # AMD64 raw image doesn't support command line args (no bootloader yet)
        if [ "$arch" = "amd64" ] && [ "$boot_type" = "image" ]; then
            skip_test_case "AMD64 raw image doesn't support command line args"
            continue
        fi

        qemu_args=(
            -append "'log=debug app=mac-virtio-net app=mac-virtio-net'"
            -device "$device,netdev=net0,mac=52:54:00:12:34:56"
            -device "$device,netdev=net1,mac=52:54:00:12:34:57"
            -netdev hubport,id=net0,hubid=0
            -netdev hubport,id=net1,hubid=0
        )
        output=$(run_test_case "$qemu_cmd ${qemu_args[*]}")
        assert_count "$output" "MAC: 52:54:00:12:34:56" 1 "First MAC address found"
        assert_count "$output" "MAC: 52:54:00:12:34:57" 1 "Second MAC address found"


        # one missing device
        test_section "virtio-net one missing device ($arch $boot_type)"

        # AMD64 raw image doesn't support command line args (no bootloader yet)
        if [ "$arch" = "amd64" ] && [ "$boot_type" = "image" ]; then
            skip_test_case "AMD64 raw image doesn't support command line args"
            continue
        fi

        qemu_args=(
            -append "'log=debug app=mac-virtio-net app=mac-virtio-net'"
            -device "$device,netdev=net0,mac=52:54:00:12:34:56"
            -netdev hubport,id=net0,hubid=0
        )
        output=$(run_test_case "$qemu_cmd ${qemu_args[*]}")
        assert_count "$output" "MAC: 52:54:00:12:34:56" 1 "First MAC address found"
        assert_count "$output" "MAC: 52:54:00:12:34:57" 0 "Second MAC address not found"
    done
done

finish_test_matrix "network driver tests"
