#!/bin/bash


# Test RTL8139 MAC address reading
# Usage: ./apps/netdev-mac/mac_rtl8139.test.sh [-v] [--arch=riscv|arm64|amd64] [--boot=kernel|image|iso]
#   -v: verbose mode (prints QEMU output)
#   --arch=riscv|arm64|amd64: specify architecture (default: all, comma-separated supported)
#   --boot=kernel|image|iso: specify boot type (default: all, comma-separated supported)
#
# Examples:
#   ./apps/netdev-mac/mac_rtl8139.test.sh              # Run all architectures
#   ./apps/netdev-mac/mac_rtl8139.test.sh -v           # Run all with verbose output
#   ./apps/netdev-mac/mac_rtl8139.test.sh --arch=arm64        # Run ARM64 only
#   ./apps/netdev-mac/mac_rtl8139.test.sh -v --arch=riscv     # Run RISC-V with verbose output
#   ./apps/netdev-mac/mac_rtl8139.test.sh -v --arch=amd64 --boot=image
#   ./apps/netdev-mac/mac_rtl8139.test.sh --arch=riscv,amd64  # Run RISC-V and AMD64


SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$PROJECT_ROOT/tests/common.sh"

init_test_matrix "$@" "Testing RTL8139 network driver"

for arch in $TEST_MATRIX_ARCH; do
    for boot_type in $TEST_MATRIX_BOOT_TYPE; do
        test_section "rtl8139 single device ($arch $boot_type)"

        # rtl8139 only supported on AMD64
        if [ "$arch" != "amd64" ]; then
            skip_test_case "rtl8139 not supported on $arch"
            continue
        fi

        qemu_cmd=$(get_full_qemu_cmd "$arch" "$boot_type" "log=debug app=mac-rtl8139")

        # single device
        qemu_args=(
            -device "rtl8139,netdev=net0,mac=52:54:00:12:34:56"
            -netdev hubport,id=net0,hubid=0
        )
        output=$(run_test_case "$qemu_cmd ${qemu_args[*]}")
        assert_count "$output" "MAC: 52:54:00:12:34:56" 1 "MAC address found"


        # two devices
        test_section "rtl8139 two devices ($arch $boot_type)"

        # rtl8139 only supported on AMD64
        if [ "$arch" != "amd64" ]; then
            skip_test_case "rtl8139 not supported on $arch"
            continue
        fi

        qemu_cmd=$(get_full_qemu_cmd "$arch" "$boot_type" "log=debug app=mac-rtl8139 app=mac-rtl8139")
        qemu_args=(
            -device "rtl8139,netdev=net0,mac=52:54:00:12:34:56"
            -device "rtl8139,netdev=net1,mac=52:54:00:12:34:57"
            -netdev hubport,id=net0,hubid=0
            -netdev hubport,id=net1,hubid=0
        )
        output=$(run_test_case "$qemu_cmd ${qemu_args[*]}")
        assert_count "$output" "MAC: 52:54:00:12:34:56" 1 "First MAC address found"
        assert_count "$output" "MAC: 52:54:00:12:34:57" 1 "Second MAC address found"


        # one missing device
        test_section "rtl8139 one missing device ($arch $boot_type)"

        # rtl8139 only supported on AMD64
        if [ "$arch" != "amd64" ]; then
            skip_test_case "rtl8139 not supported on $arch"
            continue
        fi

        qemu_args=(
            -device "rtl8139,netdev=net0,mac=52:54:00:12:34:56"
            -netdev hubport,id=net0,hubid=0
        )
        output=$(run_test_case "$qemu_cmd ${qemu_args[*]}")
        assert_count "$output" "MAC: 52:54:00:12:34:56" 1 "First MAC address found"
        assert_count "$output" "MAC: 52:54:00:12:34:57" 0 "Second MAC address not found"
    done
done

finish_test_matrix "rtl8139 driver tests"
