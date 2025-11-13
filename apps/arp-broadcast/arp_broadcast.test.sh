#!/bin/bash

# Test arp-broadcast application - send ARP broadcast between two devices
# Usage: ./apps/arp-broadcast/arp_broadcast.test.sh [-v] [riscv]
#
# Examples:
#   ./apps/arp-broadcast/arp_broadcast.test.sh              # Run RISC-V
#   ./apps/arp-broadcast/arp_broadcast.test.sh -v           # Run with verbose output

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$PROJECT_ROOT/tests/common.sh"

init_test_matrix "$@" "Testing arp-broadcast application"

for arch in $TEST_MATRIX_ARCH; do
    case "$arch" in
        riscv)
            # RISC-V kernel with 2 virtio-net devices on same hub
            test_section "arp-broadcast: RISC-V with 2 virtio-net"

            qemu_cmd=$(get_full_qemu_cmd "riscv" "kernel")
            qemu_args=(
                -append "'app=arp-broadcast'"
                -device "virtio-net-device,netdev=net0,mac=52:54:00:12:34:56"
                -device "virtio-net-device,netdev=net1,mac=52:54:00:12:34:57"
                -device "virtio-net-device,netdev=net2,mac=52:54:00:12:34:58"
                -netdev hubport,id=net0,hubid=0
                -netdev hubport,id=net1,hubid=0
                -netdev hubport,id=net2,hubid=0
            )
            output=$(run_test_case "$qemu_cmd ${qemu_args[*]}")
            assert_count "$output" "virtio-net@0.1.0] MAC: 52:54:00:12:34:56" 1 "First virtio-net MAC found"
            assert_count "$output" "virtio-net@0.1.0] MAC: 52:54:00:12:34:57" 1 "Second virtio-net MAC found"
            assert_count "$output" "virtio-net@0.1.0] MAC: 52:54:00:12:34:58" 1 "Third virtio-net MAC found"
            assert_count "$output" "TX: Building ARP broadcast" 1 "ARP broadcast built"
            assert_count "$output" "TX: Packet sent successfully" 1 "Packet transmitted"
            assert_count "$output" "RX: Packet received" 2 "Packet received by 2 devices"
            assert_count "$output" "ARP who-has 10.0.2.15 tell 10.0.2.1" 3 "ARP request (TX and 2x RX)"
            ;;
    esac
done

finish_test_matrix "arp-broadcast application tests"
