#!/bin/bash

# Test kernel command line argument parsing
# Usage: ./kernel/tests/kernel-command-line.test.sh [-v] [--arch=riscv|arm64|amd64] [--boot=kernel|image|iso]
#   -v: verbose mode (prints QEMU output)
#   --arch=riscv|arm64|amd64: specify architecture (default: all, comma-separated supported)
#   --boot=kernel|image|iso: specify boot type (default: all, comma-separated supported)
#
# Examples:
#   ./kernel/tests/kernel-command-line.test.sh              # Run all tests
#   ./kernel/tests/kernel-command-line.test.sh -v           # Run all tests with verbose output
#   ./kernel/tests/kernel-command-line.test.sh --arch=amd64        # Run AMD64 tests (kernel and image)
#   ./kernel/tests/kernel-command-line.test.sh --arch=amd64 --boot=kernel # Run AMD64 kernel test only
#   ./kernel/tests/kernel-command-line.test.sh -v --arch=arm64     # Run ARM64 tests with verbose output
#   ./kernel/tests/kernel-command-line.test.sh --arch=arm64 --boot=image  # Run ARM64 disk image test
#   ./kernel/tests/kernel-command-line.test.sh --arch=riscv,amd64 --boot=kernel  # Run RISC-V and AMD64 kernel tests

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$PROJECT_ROOT/tests/common.sh"


CMDLINE_ARGS="lorem ipsum"

init_test_matrix "$@" "Testing kernel command line parsing with: '$CMDLINE_ARGS'"

for arch in $TEST_MATRIX_ARCH; do
    for boot_type in $TEST_MATRIX_BOOT_TYPE; do
        test_section "kernel command line ($arch $boot_type)"

        qemu_cmd=$(get_full_qemu_cmd "$arch" "$boot_type" "log=debug $CMDLINE_ARGS")
        output=$(run_test_case "$qemu_cmd")
        assert_count "$output" "lorem ipsum" 1 "Command line '$CMDLINE_ARGS' found in output"
    done
done

finish_test_matrix "kernel command line tests"
