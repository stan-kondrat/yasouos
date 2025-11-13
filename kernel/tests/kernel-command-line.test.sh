#!/bin/bash

# Test kernel command line argument parsing
# Usage: ./kernel/tests/kernel-command-line.test.sh [-v] [arch] [boot_type]
#   -v: verbose mode (prints QEMU output)
#   arch: riscv|arm64|amd64 (if not specified, runs all)
#   boot_type: kernel|image (if not specified, runs both)
#
# Examples:
#   ./kernel/tests/kernel-command-line.test.sh              # Run all tests
#   ./kernel/tests/kernel-command-line.test.sh -v           # Run all tests with verbose output
#   ./kernel/tests/kernel-command-line.test.sh amd64        # Run AMD64 tests (kernel and image)
#   ./kernel/tests/kernel-command-line.test.sh amd64 kernel # Run AMD64 kernel test only
#   ./kernel/tests/kernel-command-line.test.sh -v arm64     # Run ARM64 tests with verbose output
#   ./kernel/tests/kernel-command-line.test.sh arm64 image  # Run ARM64 disk image test

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$PROJECT_ROOT/tests/common.sh"


CMDLINE_ARGS="lorem ipsum"

init_test_matrix "$@" "Testing kernel command line parsing with: '$CMDLINE_ARGS'"

for arch in $TEST_MATRIX_ARCH; do
    for boot_type in $TEST_MATRIX_BOOT_TYPE; do
        test_section "kernel command line ($arch $boot_type)"

        # AMD64 raw image doesn't support command line args (no bootloader yet)
        if [ "$arch" = "amd64" ] && [ "$boot_type" = "image" ]; then
            skip_test_case "AMD64 raw image doesn't support command line args"
            continue
        fi

        qemu_cmd=$(get_full_qemu_cmd "$arch" "$boot_type")

        qemu_args=(
            -append "'$CMDLINE_ARGS'"
        )
        output=$(run_test_case "$qemu_cmd ${qemu_args[*]}")
        assert_count "$output" "lorem ipsum" 1 "Command line '$CMDLINE_ARGS' found in output"
    done
done

finish_test_matrix "kernel command line tests"
