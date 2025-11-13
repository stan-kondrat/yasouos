#!/bin/bash


# Test e1000 driver MAC address reading
# Usage: ./drivers/e1000/e1000.test.sh [-v] [riscv|arm64|amd64] [kernel|image]
#
# Examples:
#   ./drivers/e1000/e1000.test.sh              # Run all architectures
#   ./drivers/e1000/e1000.test.sh -v           # Run all with verbose output
#   ./drivers/e1000/e1000.test.sh arm64        # Run ARM64 only
#   ./drivers/e1000/e1000.test.sh -v riscv     # Run RISC-V with verbose output
#   ./drivers/e1000/e1000.test.sh -v amd64 image


SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$PROJECT_ROOT/tests/common.sh"

init_test_matrix "$@" "Testing E1000 network driver"

for arch in $TEST_MATRIX_ARCH; do
    for boot_type in $TEST_MATRIX_BOOT_TYPE; do
        test_section "e1000 single device ($arch $boot_type)"

        # e1000 only supported on RISC-V
        if [ "$arch" = "amd64" ] || [ "$arch" = "arm64" ]; then
            skip_test_case "e1000 not supported on $arch"
            continue
        fi

        qemu_cmd=$(get_full_qemu_cmd "$arch" "$boot_type")

        # single device
        qemu_args=(
            -append "'app=mac-e1000'"
            -device "e1000,netdev=net0,mac=52:54:00:12:34:56"
            -netdev hubport,id=net0,hubid=0
        )
        output=$(run_test_case "$qemu_cmd ${qemu_args[*]}")
        assert_count "$output" "MAC: 52:54:00:12:34:56" 1 "MAC address found"


        # two devices
        test_section "e1000 two devices ($arch $boot_type)"

        # e1000 only supported on RISC-V
        if [ "$arch" = "amd64" ] || [ "$arch" = "arm64" ]; then
            skip_test_case "e1000 not supported on $arch"
            continue
        fi

        qemu_args=(
            -append "'app=mac-e1000 app=mac-e1000'"
            -device "e1000,netdev=net0,mac=52:54:00:12:34:56"
            -device "e1000,netdev=net1,mac=52:54:00:12:34:57"
            -netdev hubport,id=net0,hubid=0
            -netdev hubport,id=net1,hubid=0
        )
        output=$(run_test_case "$qemu_cmd ${qemu_args[*]}")
        assert_count "$output" "MAC: 52:54:00:12:34:56" 1 "First MAC address found"
        assert_count "$output" "MAC: 52:54:00:12:34:57" 1 "Second MAC address found"


        # one missing device
        test_section "e1000 one missing device ($arch $boot_type)"

        # e1000 only supported on RISC-V
        if [ "$arch" = "amd64" ] || [ "$arch" = "arm64" ]; then
            skip_test_case "e1000 not supported on $arch"
            continue
        fi

        qemu_args=(
            -append "'app=mac-e1000 app=mac-e1000'"
            -device "e1000,netdev=net0,mac=52:54:00:12:34:56"
            -netdev hubport,id=net0,hubid=0
        )
        output=$(run_test_case "$qemu_cmd ${qemu_args[*]}")
        assert_count "$output" "MAC: 52:54:00:12:34:56" 1 "First MAC address found"
        assert_count "$output" "MAC: 52:54:00:12:34:57" 0 "Second MAC address not found"
    done
done

finish_test_matrix "e1000 driver tests"
