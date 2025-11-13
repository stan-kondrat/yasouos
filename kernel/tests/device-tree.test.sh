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

# Parse verbose flag
eval "$(parse_verbose_flag "$@")"

# Parse architectures to test
ARCHS=$(parse_arch "$1")
BOOT_TYPE_FILTER="$2"

# Only test kernel boot (skip if filter is "image")
if [ "$BOOT_TYPE_FILTER" = "image" ]; then
    echo "Skipping device tree tests (only kernel boot supported)"
    exit 0
fi

TIMEOUT=3

# Color for info messages
COLOR_CYAN='\033[0;36m'
COLOR_RESET='\033[0m'

echo -e "${COLOR_CYAN}Testing device tree enumeration${COLOR_RESET}"

run_device_tree_test() {
    local test_num="$1"
    local arch="$2"
    local image="$3"
    local desc="$4"

    echo "  Test $test_num: $desc"

    check_image_exists "$image" "$arch" || return 1

    test_timer_start
    output=$(run_qemu_test "$arch" "$image" "test=hello debug loglevel=7" "$TIMEOUT")

    if [ "$VERBOSE" -eq 1 ]; then
        echo "    --- QEMU Output ---"
        echo "$output" | sed 's/^/    /'
        echo "    --- End Output ---"
    fi

    # Check for device tree output
    if ! echo "$output" | grep -q "Device tree:"; then
        test_fail "Device tree output not found"
        return 1
    fi

    # Check for device entries (should have at least one device with @ address)
    if ! echo "$output" | grep -q "@ 0x"; then
        test_fail "No devices found in tree (expected '@ 0x' pattern)"
        return 1
    fi

    test_pass "Device tree enumeration working"
    return 0
}

test_count=0
failed_count=0

for arch in $ARCHS; do
    test_section "Testing $arch"

    kernel=$(get_kernel_path "$arch")

    test_count=$((test_count + 1))
    run_device_tree_test "$test_count" "$arch" "$kernel" "$arch device tree" || failed_count=$((failed_count + 1))
done

echo ""
if [ $failed_count -eq 0 ]; then
    echo -e "${COLOR_BOLD}${COLOR_GREEN}=== All $test_count device tree tests passed ===${COLOR_RESET}"
else
    echo -e "${COLOR_BOLD}${COLOR_RED}=== $failed_count of $test_count device tree tests failed ===${COLOR_RESET}"
    exit 1
fi
