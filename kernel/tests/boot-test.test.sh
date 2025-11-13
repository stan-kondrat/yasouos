#!/bin/bash

# Test basic kernel boot by checking for version string
# Usage: ./kernel/tests/boot-test.test.sh [-v] [arch] [boot_type]
#   -v: verbose mode (prints QEMU output)
#   arch: riscv|arm64|amd64 (if not specified, runs all)
#   boot_type: kernel|image (if not specified, runs both)
#
# Examples:
#   ./kernel/tests/boot-test.test.sh              # Run all 5 tests
#   ./kernel/tests/boot-test.test.sh -v           # Run all tests with verbose output
#   ./kernel/tests/boot-test.test.sh amd64        # Run AMD64 kernel test
#   ./kernel/tests/boot-test.test.sh amd64 kernel # Run AMD64 kernel test
#   ./kernel/tests/boot-test.test.sh -v arm64     # Run ARM64 tests with verbose output
#   ./kernel/tests/boot-test.test.sh arm64 image  # Run ARM64 disk image test
#
# Tests 5 boot configurations:
# 1. AMD64 kernel boot
# 2. ARM64 kernel boot
# 3. ARM64 disk image boot
# 4. RISC-V kernel boot
# 5. RISC-V disk image boot

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$PROJECT_ROOT/tests/common.sh"

# Parse verbose flag
eval "$(parse_verbose_flag "$@")"

# Parse architectures to test
ARCHS=$(parse_arch "$1")
BOOT_TYPE_FILTER="$2"

BOOT_STRING="YasouOS v0."
TIMEOUT=3

# Color for info messages
COLOR_CYAN='\033[0;36m'
COLOR_RESET='\033[0m'

echo -e "${COLOR_CYAN}Testing kernel boot - checking for: '$BOOT_STRING'${COLOR_RESET}"

run_boot_test() {
    local test_num="$1"
    local arch="$2"
    local image="$3"
    local boot_type="$4"
    local desc="$5"

    echo "  Test $test_num: $desc ($boot_type)"

    check_image_exists "$image" "$arch" || return 1

    test_timer_start
    output=$(run_qemu_test "$arch" "$image" "test=hello debug loglevel=7" "$TIMEOUT")

    if [ "$VERBOSE" -eq 1 ]; then
        echo "    --- QEMU Output ---"
        echo "$output" | sed 's/^/    /'
        echo "    --- End Output ---"
    fi

    if echo "$output" | grep -q "$BOOT_STRING"; then
        test_pass "Boot string '$BOOT_STRING' found in output"
        return 0
    else
        test_fail "Boot string '$BOOT_STRING' NOT found in output"
        return 1
    fi
}

test_count=0
failed_count=0

for arch in $ARCHS; do
    test_section "Testing $arch"

    kernel=$(get_kernel_path "$arch")
    disk=$(get_disk_path "$arch")

    case "$arch" in
        amd64)
            # Test 1: AMD64 kernel boot
            if [ -z "$BOOT_TYPE_FILTER" ] || [ "$BOOT_TYPE_FILTER" = "kernel" ]; then
                test_count=$((test_count + 1))
                run_boot_test "$test_count" "$arch" "$kernel" "kernel" "AMD64" || failed_count=$((failed_count + 1))
            fi

            # Test 2: AMD64 disk image boot
            if [ -z "$BOOT_TYPE_FILTER" ] || [ "$BOOT_TYPE_FILTER" = "image" ]; then
                test_count=$((test_count + 1))
                run_boot_test "$test_count" "$arch" "$disk" "image" "AMD64" || failed_count=$((failed_count + 1))
            fi
            ;;
        arm64)
            # Test 3: ARM64 kernel boot
            if [ -z "$BOOT_TYPE_FILTER" ] || [ "$BOOT_TYPE_FILTER" = "kernel" ]; then
                test_count=$((test_count + 1))
                run_boot_test "$test_count" "$arch" "$kernel" "kernel" "ARM64" || failed_count=$((failed_count + 1))
            fi

            # Test 4: ARM64 disk image boot
            if [ -z "$BOOT_TYPE_FILTER" ] || [ "$BOOT_TYPE_FILTER" = "image" ]; then
                test_count=$((test_count + 1))
                run_boot_test "$test_count" "$arch" "$disk" "image" "ARM64" || failed_count=$((failed_count + 1))
            fi
            ;;
        riscv)
            # Test 5: RISC-V kernel boot
            if [ -z "$BOOT_TYPE_FILTER" ] || [ "$BOOT_TYPE_FILTER" = "kernel" ]; then
                test_count=$((test_count + 1))
                run_boot_test "$test_count" "$arch" "$kernel" "kernel" "RISC-V" || failed_count=$((failed_count + 1))
            fi

            # Test 6: RISC-V disk image boot
            if [ -z "$BOOT_TYPE_FILTER" ] || [ "$BOOT_TYPE_FILTER" = "image" ]; then
                test_count=$((test_count + 1))
                run_boot_test "$test_count" "$arch" "$disk" "image" "RISC-V" || failed_count=$((failed_count + 1))
            fi
            ;;
    esac
done

echo ""
# Use colors from _common.sh
if [ $failed_count -eq 0 ]; then
    echo -e "${COLOR_BOLD}${COLOR_GREEN}=== All $test_count boot tests passed ===${COLOR_RESET}"
else
    echo -e "${COLOR_BOLD}${COLOR_RED}=== $failed_count of $test_count boot tests failed ===${COLOR_RESET}"
    exit 1
fi
