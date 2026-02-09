#!/bin/bash

# Test device tree enumeration and printing
# Usage: ./kernel/tests/device-tree.test.sh [-v] [arch] [boot_type]
#   -v: verbose mode (prints QEMU output)
#   arch: riscv|arm64|amd64 (if not specified, runs all)
#   boot_type: kernel|image (only kernel is tested, image is ignored)
#
# Examples:
#   ./kernel/tests/device-tree.test.sh              # Run all architectures
#   ./kernel/tests/device-tree.test.sh -v           # Run all with verbose output
#   ./kernel/tests/device-tree.test.sh arm64        # Run ARM64 only
#   ./kernel/tests/device-tree.test.sh -v riscv     # Run RISC-V with verbose output

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$PROJECT_ROOT/tests/common.sh"

init_test_matrix "$@" "Testing device tree enumeration"

for arch in $TEST_MATRIX_ARCH; do
    for boot_type in $TEST_MATRIX_BOOT_TYPE; do
        test_section "device tree enumeration ($arch $boot_type)"

        # AMD64 raw image doesn't support command line args (no bootloader yet)
        if [ "$arch" = "amd64" ] && [ "$boot_type" = "image" ]; then
            skip_test_case "AMD64 raw image doesn't support command line args"
            continue
        fi

        qemu_cmd=$(get_full_qemu_cmd "$arch" "$boot_type")

        qemu_args=(
            -append "'log=debug test=hello'"
        )
        output=$(run_test_case "$qemu_cmd ${qemu_args[*]}")

        # Increment test count manually since we're not using assert_count
        TEST_MATRIX_TEST_COUNT=$((TEST_MATRIX_TEST_COUNT + 1))

        # Check for device tree header
        if ! echo "$output" | grep -q "Device tree:"; then
            TEST_MATRIX_FAILED_COUNT=$((TEST_MATRIX_FAILED_COUNT + 1))
            echo -e "  ${COLOR_RED}✗${COLOR_RESET} Device tree output not found"
            continue
        fi

        # Check for at least one device (should have devices with @ address)
        if ! echo "$output" | grep -q "@ 0x"; then
            TEST_MATRIX_FAILED_COUNT=$((TEST_MATRIX_FAILED_COUNT + 1))
            echo -e "  ${COLOR_RED}✗${COLOR_RESET} No devices found in tree (expected '@ 0x' pattern)"
            continue
        fi

        echo -e "  ${COLOR_GREEN}✓${COLOR_RESET} Device tree enumeration working"
    done
done

finish_test_matrix "device tree tests"
