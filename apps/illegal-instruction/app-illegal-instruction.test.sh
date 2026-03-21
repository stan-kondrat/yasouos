#!/bin/bash

# Test illegal instruction exception handling
# Usage: ./apps/illegal-instruction/app-illegal-instruction.test.sh [-v] [--arch=riscv|arm64|amd64] [--boot=kernel|image|iso]
#   -v: verbose mode (prints QEMU output)
#   --arch=riscv|arm64|amd64: specify architecture (default: all, comma-separated supported)
#   --boot=kernel|image|iso: specify boot type (default: all, comma-separated supported)
#
# Examples:
#   ./apps/illegal-instruction/app-illegal-instruction.test.sh              # Run all tests
#   ./apps/illegal-instruction/app-illegal-instruction.test.sh -v           # Run all tests with verbose output
#   ./apps/illegal-instruction/app-illegal-instruction.test.sh --arch=amd64        # Run AMD64 test only
#   ./apps/illegal-instruction/app-illegal-instruction.test.sh -v --arch=arm64     # Run ARM64 test with verbose output
#   ./apps/illegal-instruction/app-illegal-instruction.test.sh --arch=amd64,arm64  # Run AMD64 and ARM64

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$PROJECT_ROOT/tests/common.sh"

init_test_matrix "$@" "Testing illegal instruction exception with: 'app=illegal-instruction'"

for arch in $TEST_MATRIX_ARCH; do
    for boot_type in $TEST_MATRIX_BOOT_TYPE; do
        test_section "illegal instruction exception ($arch $boot_type)"

        qemu_cmd=$(get_full_qemu_cmd "$arch" "$boot_type" "log=debug app=illegal-instruction")
        output=$(run_test_case "$qemu_cmd")
        assert_count "$output" "\\[EXCEPTION\\] Unknown/Illegal Instruction" 1 "Exception message found in output"
    done
done

finish_test_matrix "illegal instruction tests"
