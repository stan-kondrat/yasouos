#!/bin/bash

# Test illegal instruction exception handling
# Usage: ./apps/illegal-instruction/app-illegal-instruction.test.sh [-v] [arch] [boot_type]
#   -v: verbose mode (prints QEMU output)
#   arch: riscv|arm64|amd64 (if not specified, runs all)
#   boot_type: kernel|image (only kernel is tested, image is ignored)
#
# Examples:
#   ./apps/illegal-instruction/app-illegal-instruction.test.sh              # Run all tests
#   ./apps/illegal-instruction/app-illegal-instruction.test.sh -v           # Run all tests with verbose output
#   ./apps/illegal-instruction/app-illegal-instruction.test.sh amd64        # Run AMD64 test only
#   ./apps/illegal-instruction/app-illegal-instruction.test.sh -v arm64     # Run ARM64 test with verbose output

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$PROJECT_ROOT/tests/common.sh"

# Parse verbose flag
eval "$(parse_verbose_flag "$@")"

# Parse architectures to test
ARCHS=$(parse_arch "$1")
BOOT_TYPE_FILTER="$2"

# Only test kernel boot (skip if filter is "image")
if [ "$BOOT_TYPE_FILTER" = "image" ]; then
    echo "Skipping illegal instruction tests (only kernel boot supported)"
    exit 0
fi

CMDLINE_ARGS="app=illegal-instruction"
TIMEOUT=3

# Color for info messages
COLOR_CYAN='\033[0;36m'
COLOR_RESET='\033[0m'

echo -e "${COLOR_CYAN}Testing illegal instruction exception with: '$CMDLINE_ARGS'${COLOR_RESET}"

run_exception_test() {
    local test_num="$1"
    local arch="$2"
    local image="$3"
    local desc="$4"

    echo "  Test $test_num: $desc"

    check_image_exists "$image" "$arch" || return 1

    test_timer_start
    output=$(run_qemu_test "$arch" "$image" "$CMDLINE_ARGS" "$TIMEOUT")

    if [ "$VERBOSE" -eq 1 ]; then
        echo "    --- QEMU Output ---"
        echo "$output" | sed 's/^/    /'
        echo "    --- End Output ---"
    fi

    if echo "$output" | grep -q "\[EXCEPTION\] Unknown/Illegal Instruction"; then
        test_pass "Exception message found in output"
        return 0
    else
        test_fail "Exception message NOT found in output"
        return 1
    fi
}

test_count=0
failed_count=0

for arch in $ARCHS; do
    test_section "Testing $arch"

    kernel=$(get_kernel_path "$arch")

    test_count=$((test_count + 1))
    run_exception_test "$test_count" "$arch" "$kernel" "$arch" || failed_count=$((failed_count + 1))
done

echo ""
# Use colors from _common.sh
if [ $failed_count -eq 0 ]; then
    echo -e "${COLOR_BOLD}${COLOR_GREEN}=== All $test_count illegal instruction tests passed ===${COLOR_RESET}"
else
    echo -e "${COLOR_BOLD}${COLOR_RED}=== $failed_count of $test_count illegal instruction tests failed ===${COLOR_RESET}"
    exit 1
fi
