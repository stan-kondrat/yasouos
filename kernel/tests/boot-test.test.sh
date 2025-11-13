#!/bin/bash

# Test basic kernel boot by checking for version string
# Usage: ./kernel/tests/boot-test.test.sh [-v] [riscv|arm64|amd64] [kernel|image]
#
# Examples:
#   ./kernel/tests/boot-test.test.sh              # Run all tests
#   ./kernel/tests/boot-test.test.sh -v           # Run all tests with verbose output
#   ./kernel/tests/boot-test.test.sh amd64        # Run AMD64 tests (kernel and image)
#   ./kernel/tests/boot-test.test.sh amd64 kernel # Run AMD64 kernel test only
#   ./kernel/tests/boot-test.test.sh -v arm64     # Run ARM64 tests with verbose output
#   ./kernel/tests/boot-test.test.sh arm64 image  # Run ARM64 disk image test

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$PROJECT_ROOT/tests/common.sh"

BOOT_STRING="YasouOS v0."

init_test_matrix "$@" "Testing kernel boot"

for arch in $TEST_MATRIX_ARCH; do
    for boot_type in $TEST_MATRIX_BOOT_TYPE; do
        qemu_cmd=$(get_full_qemu_cmd "$arch" "$boot_type")

        test_section "boot test ($arch $boot_type)"

        output=$(run_test_case "$qemu_cmd")
        assert_count "$output" "$BOOT_STRING" 1 "Boot string '$BOOT_STRING' found"
    done
done

finish_test_matrix "boot tests"
