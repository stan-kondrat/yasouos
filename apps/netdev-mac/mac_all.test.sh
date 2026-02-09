#!/bin/bash

# Test mac-all application - prints all network devices MAC addresses
# Usage: ./apps/netdev-mac/mac_all.test.sh [-v] [riscv|arm64|amd64]
#
# Examples:
#   ./apps/netdev-mac/mac_all.test.sh              # Run all architectures
#   ./apps/netdev-mac/mac_all.test.sh -v           # Run all with verbose output
#   ./apps/netdev-mac/mac_all.test.sh riscv        # Run RISC-V only
#   ./apps/netdev-mac/mac_all.test.sh -v amd64     # Run AMD64 with verbose output

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$PROJECT_ROOT/tests/common.sh"

init_test_matrix "$@" "Testing mac-all application"

for arch in $TEST_MATRIX_ARCH; do
    case "$arch" in
        riscv)
            # RISC-V kernel with 2 virtio-net and 1 e1000
            test_section "mac-all: RISC-V with 2 virtio-net + 1 e1000"

            qemu_cmd=$(get_full_qemu_cmd "riscv" "kernel")
            qemu_args=(
                -append "'log=debug app=mac-all'"
                -device "virtio-net-device,netdev=net0,mac=52:54:00:11:11:11"
                -device "virtio-net-device,netdev=net1,mac=52:54:00:22:22:22"
                -device "e1000,netdev=net2,mac=52:54:00:33:33:33"
                -netdev hubport,id=net0,hubid=0
                -netdev hubport,id=net1,hubid=0
                -netdev hubport,id=net2,hubid=0
            )
            output=$(run_test_case "$qemu_cmd ${qemu_args[*]}")
            assert_count "$output" "virtio-net@0.1.0] MAC: 52:54:00:11:11:11" 1 "First virtio-net MAC found"
            assert_count "$output" "virtio-net@0.1.0] MAC: 52:54:00:22:22:22" 1 "Second virtio-net MAC found"
            assert_count "$output" "e1000@0.1.0] MAC: 52:54:00:33:33:33" 1 "E1000 MAC found"
            ;;

        amd64)
            # AMD64 kernel with 1 virtio-net and 1 rtl8139
            test_section "mac-all: AMD64 with 1 virtio-net + 1 rtl8139"

            qemu_cmd=$(get_full_qemu_cmd "amd64" "kernel")
            qemu_args=(
                -append "'log=debug app=mac-all'"
                -device "virtio-net-pci,netdev=net0,mac=52:54:00:AA:BB:CC"
                -device "rtl8139,netdev=net1,mac=52:54:00:DD:EE:FF"
                -netdev hubport,id=net0,hubid=0
                -netdev hubport,id=net1,hubid=0
            )
            output=$(run_test_case "$qemu_cmd ${qemu_args[*]}")
            assert_count "$output" "virtio-net@0.1.0] MAC: 52:54:00:aa:bb:cc" 1 "VirtIO-Net MAC found"
            assert_count "$output" "rtl8139@0.1.0] MAC: 52:54:00:dd:ee:ff" 1 "RTL8139 MAC found"
            ;;

        arm64)
            # ARM64 kernel with 3 virtio-net
            test_section "mac-all: ARM64 with 3 virtio-net"

            qemu_cmd=$(get_full_qemu_cmd "arm64" "kernel")
            qemu_args=(
                -append "'log=debug app=mac-all'"
                -device "virtio-net-device,netdev=net0,mac=52:54:00:01:02:03"
                -device "virtio-net-device,netdev=net1,mac=52:54:00:04:05:06"
                -device "virtio-net-device,netdev=net2,mac=52:54:00:07:08:09"
                -netdev hubport,id=net0,hubid=0
                -netdev hubport,id=net1,hubid=0
                -netdev hubport,id=net2,hubid=0
            )
            output=$(run_test_case "$qemu_cmd ${qemu_args[*]}")
            assert_count "$output" "virtio-net@0.1.0] MAC: 52:54:00:01:02:03" 1 "First virtio-net MAC found"
            assert_count "$output" "virtio-net@0.1.0] MAC: 52:54:00:04:05:06" 1 "Second virtio-net MAC found"
            assert_count "$output" "virtio-net@0.1.0] MAC: 52:54:00:07:08:09" 1 "Third virtio-net MAC found"
            ;;
    esac
done

finish_test_matrix "mac-all application tests"
