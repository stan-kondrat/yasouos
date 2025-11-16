#!/bin/bash

# Common test utilities for YasouOS tests - Matrix-based API

# Parse verbose flag and detect CI environment
# Usage: eval "$(parse_verbose_flag "$@")"
# Outputs: VERBOSE=1 and optional shift command
parse_verbose_flag() {
    if [ -n "$CI" ] || [ -n "$GITHUB_ACTIONS" ]; then
        if [ "$1" = "-v" ]; then
            echo "VERBOSE=1; shift"
        else
            echo "VERBOSE=1"
        fi
    elif [ "$1" = "-v" ]; then
        echo "VERBOSE=1; shift"
    else
        echo "VERBOSE=0"
    fi
}

# Parse ARCH from command line or run all architectures
parse_arch() {
    local arch_arg="$1"

    if [ -z "$arch_arg" ]; then
        echo "riscv arm64 amd64"
        return
    fi

    case "$arch_arg" in
        ARCH=riscv|riscv)
            echo "riscv"
            ;;
        ARCH=arm64|arm64)
            echo "arm64"
            ;;
        ARCH=amd64|amd64)
            echo "amd64"
            ;;
        *)
            echo "Error: Invalid architecture '$arch_arg'. Use ARCH=riscv, ARCH=arm64, or ARCH=amd64" >&2
            exit 1
            ;;
    esac
}

# Get QEMU command for architecture
get_qemu_cmd() {
    local arch="$1"
    case "$arch" in
        riscv)
            echo "qemu-system-riscv64"
            ;;
        arm64)
            echo "qemu-system-aarch64"
            ;;
        amd64)
            echo "qemu-system-x86_64"
            ;;
        *)
            echo "Error: Unknown architecture '$arch'" >&2
            return 1
            ;;
    esac
}

# Get QEMU machine flags for architecture
get_qemu_machine_flags() {
    local arch="$1"
    case "$arch" in
        riscv)
            echo "-machine virt -bios default"
            ;;
        arm64)
            echo "-machine virt -cpu cortex-a53"
            ;;
        amd64)
            echo "-machine pc -cpu qemu64 -m 128M -device isa-debug-exit,iobase=0xf4,iosize=0x04 "
            ;;
        *)
            echo "Error: Unknown architecture '$arch'" >&2
            return 1
            ;;
    esac
}

# Get kernel path for architecture
get_kernel_path() {
    local arch="$1"
    case "$arch" in
        riscv)
            echo "build/riscv/kernel.elf"
            ;;
        arm64)
            echo "build/arm64/kernel.bin"
            ;;
        amd64)
            echo "build/amd64/kernel.elf"
            ;;
        *)
            echo "Error: Unknown architecture '$arch'" >&2
            return 1
            ;;
    esac
}

# Get disk image path for architecture
get_disk_path() {
    local arch="$1"
    case "$arch" in
        riscv)
            echo "build/riscv/disk.img"
            ;;
        arm64)
            echo "build/arm64/disk.img"
            ;;
        amd64)
            echo "build/amd64/disk.img"
            ;;
        *)
            echo "Error: Unknown architecture '$arch'" >&2
            return 1
            ;;
    esac
}

# Color codes
COLOR_RESET='\033[0m'
COLOR_GREEN='\033[0;32m'
COLOR_RED='\033[0;31m'
COLOR_BLUE='\033[0;34m'
COLOR_YELLOW='\033[0;33m'
COLOR_BOLD='\033[1m'
COLOR_GRAY='\033[0;90m'

# Timer functions
TEST_START_TIME=0

get_timestamp_ms() {
    if command -v gdate >/dev/null 2>&1; then
        gdate +%s%3N
    elif date +%s%3N 2>/dev/null | grep -q '^[0-9]\+$'; then
        date +%s%3N
    else
        echo $(($(date +%s) * 1000))
    fi
}

test_timer_start() {
    TEST_START_TIME=$(get_timestamp_ms)
}

test_timer_stop() {
    local end_time=$(get_timestamp_ms)
    local elapsed=$((end_time - TEST_START_TIME))
    echo "$elapsed"
}

# Format milliseconds to human readable
format_time() {
    local ms=$1
    if [ $ms -lt 1000 ]; then
        echo "${ms}ms"
    else
        local seconds=$((ms / 1000))
        local remaining_ms=$((ms % 1000))
        printf "%d.%03ds" $seconds $remaining_ms
    fi
}

TEST_SECTION_COUNT=0

test_section() {
    TEST_SECTION_COUNT=$((TEST_SECTION_COUNT + 1))
    echo ""
    echo -e "${COLOR_BOLD}${COLOR_BLUE}=== Test $TEST_SECTION_COUNT: $1 ===${COLOR_RESET}"
}

# Test matrix framework variables
TEST_MATRIX_ARCH=""
TEST_MATRIX_BOOT_TYPE=""
TEST_MATRIX_NET_DEVICE=""
TEST_MATRIX_VERBOSE=0
TEST_MATRIX_TEST_COUNT=0
TEST_MATRIX_FAILED_COUNT=0
TEST_MATRIX_TIMEOUT=5
COLOR_CYAN='\033[0;36m'

# Initialize test matrix framework
# Usage: init_test_matrix "$@" ["optional custom message"]
# Parses command line arguments for verbose flag, architecture, boot type, and network device
# Optionally displays a custom message before parsing
init_test_matrix() {
    local message=""
    local args=("$@")

    # Check if last argument is a message (not a flag or arch/boot_type/net_device)
    local last_arg="${args[${#args[@]}-1]}"
    if [ "${#args[@]}" -gt 0 ] && [ "$last_arg" != "-v" ] && \
       [ "$last_arg" != "kernel" ] && [ "$last_arg" != "image" ] && \
       [ "$last_arg" != "riscv" ] && [ "$last_arg" != "arm64" ] && [ "$last_arg" != "amd64" ] && \
       [ "$last_arg" != "e1000" ] && [ "$last_arg" != "rtl8139" ] && [ "$last_arg" != "virtio-net" ]; then
        message="$last_arg"
        unset 'args[${#args[@]}-1]'
    fi

    eval "$(parse_verbose_flag "${args[@]}")"
    TEST_MATRIX_VERBOSE=$VERBOSE

    # Remove -v flag from args array if present
    if [ "${args[0]}" = "-v" ]; then
        args=("${args[@]:1}")
    fi

    local arch_arg="${args[0]}"
    local boot_type_arg="${args[1]}"
    local net_device_arg="${args[2]}"

    # Display custom message if provided
    if [ -n "$message" ]; then
        echo -e "${COLOR_CYAN}${message}${COLOR_RESET}"
    fi

    # Parse architecture (default: all architectures)
    if [ -z "$arch_arg" ]; then
        TEST_MATRIX_ARCH="amd64 arm64 riscv"
    else
        TEST_MATRIX_ARCH=$(parse_arch "$arch_arg")
    fi

    # Parse boot type (default: both kernel and image)
    if [ -z "$boot_type_arg" ]; then
        TEST_MATRIX_BOOT_TYPE="kernel image"
    elif [ "$boot_type_arg" = "kernel" ] || [ "$boot_type_arg" = "image" ]; then
        TEST_MATRIX_BOOT_TYPE="$boot_type_arg"
    else
        TEST_MATRIX_BOOT_TYPE="kernel image"
    fi

    # Store the network device argument for later use with get_net_devices_for_arch
    TEST_MATRIX_NET_DEVICE="$net_device_arg"
}

# Get network devices for a specific architecture
# Usage: get_net_devices_for_arch <arch> <net_device_arg>
# Returns: Space-separated list of network devices for the architecture
get_net_devices_for_arch() {
    local arch="$1"
    local net_device_arg="$2"
    local virtio_device=$([ "$arch" = "amd64" ] && echo "virtio-net-pci" || echo "virtio-net-device")
    local all_devices="e1000 rtl8139 $virtio_device"

    case "$net_device_arg" in
        e1000|rtl8139)
            echo "$net_device_arg"
            ;;
        virtio-net)
            echo "$virtio_device"
            ;;
        *)
            echo "$all_devices"
            ;;
    esac
}

# Build full QEMU command with machine flags, kernel/image, and common flags
# Usage: get_full_qemu_cmd <arch> <boot_type>
# Returns: Complete QEMU command string ready to execute
get_full_qemu_cmd() {
    local arch="$1"
    local boot_type="$2"

    local qemu_binary
    qemu_binary=$(get_qemu_cmd "$arch")

    local machine_flags
    machine_flags=$(get_qemu_machine_flags "$arch")

    if [ "$boot_type" = "image" ]; then
        local disk
        disk=$(get_disk_path "$arch")
        if [ "$arch" = "amd64" ]; then
            echo "$qemu_binary $machine_flags -drive file=$disk,format=raw -nographic --no-reboot"
        else
            # For RISC-V and ARM64, disk images need kernel flag (no bootloader yet)
            echo "$qemu_binary $machine_flags -kernel $disk -nographic --no-reboot"
        fi
    else
        local kernel
        kernel=$(get_kernel_path "$arch")
        echo "$qemu_binary $machine_flags -kernel $kernel -nographic --no-reboot"
    fi
}

# Run single test case and return output
# Usage: output=$(run_test_case "<qemu_command>" [timeout])
# The function prints test metadata to stderr and returns test output to stdout
run_test_case() {
    local full_qemu_command="$1"
    local timeout="${2:-$TEST_MATRIX_TIMEOUT}"

    test_timer_start

    local tmpfile
    tmpfile=$(mktemp)

    bash -c "$full_qemu_command" > "${tmpfile}" 2>&1 &
    local qemu_pid=$!

    # Wait for QEMU with timeout (in 100ms intervals)
    local count=0
    local max_count=$((timeout * 10))
    while kill -0 "$qemu_pid" 2>/dev/null && [ $count -lt $max_count ]; do
        sleep 0.1
        count=$((count + 1))
    done

    # Kill QEMU if still running
    if kill -0 "$qemu_pid" 2>/dev/null; then
        kill -TERM "$qemu_pid" 2>/dev/null || true
        sleep 0.2
        kill -KILL "$qemu_pid" 2>/dev/null || true
    fi

    wait "$qemu_pid" 2>/dev/null || true

    # Print execution time
    local elapsed
    elapsed=$(test_timer_stop)
    local time_str
    time_str=$(format_time "$elapsed")
    echo -e "  ${COLOR_GRAY}Test execution time: ${time_str}${COLOR_RESET}" >&2

    # Clean ANSI escape sequences from output
    local output
    output=$(sed 's/\x1b\[[0-9;?]*[mGKHJABCDsuhl]//g' "${tmpfile}" | \
        sed 's/\x1b[()][0AB]//g' | \
        sed 's/\x1b[=>]//g' | \
        tr -d '\000-\010\013\014\016-\037\177')

    rm -f "${tmpfile}"

    # Print verbose output if enabled
    if [ "$TEST_MATRIX_VERBOSE" -eq 1 ]; then
        echo "  --- QEMU Output ---" >&2
        echo "$output" | sed 's/^/  /' >&2
        echo "  --- End Output ---" >&2
    fi

    echo "$output"
}

# Skip test case with a message
# Usage: skip_test_case "reason for skipping"
skip_test_case() {
    local reason="$1"
    echo -e "  ${COLOR_YELLOW}⊘${COLOR_RESET} $reason (skipping)"
}

# Assert string count in output
# Usage: assert_count "$output" "pattern" expected_count "description"
assert_count() {
    local output="$1"
    local search_pattern="$2"
    local expected_count="$3"
    local assert_desc="$4"

    # Increment test count in parent shell (not in subshell from run_test_case)
    TEST_MATRIX_TEST_COUNT=$((TEST_MATRIX_TEST_COUNT + 1))

    local actual_count
    actual_count=$(echo "$output" | grep -c "$search_pattern" || true)

    if [ "$actual_count" -eq "$expected_count" ]; then
        echo -e "  ${COLOR_GREEN}✓${COLOR_RESET} $assert_desc (found $expected_count)"
        return 0
    else
        echo -e "  ${COLOR_RED}✗${COLOR_RESET} $assert_desc (expected $expected_count, got $actual_count)"
        TEST_MATRIX_FAILED_COUNT=$((TEST_MATRIX_FAILED_COUNT + 1))
        return 1
    fi
}

# Assert string exists in output (at least once, ignoring duplicates)
# Usage: assert_contains "$output" "pattern" "description"
assert_contains() {
    local output="$1"
    local search_pattern="$2"
    local assert_desc="$3"

    # Increment test count in parent shell (not in subshell from run_test_case)
    TEST_MATRIX_TEST_COUNT=$((TEST_MATRIX_TEST_COUNT + 1))

    if echo "$output" | grep -q "$search_pattern"; then
        echo -e "  ${COLOR_GREEN}✓${COLOR_RESET} $assert_desc"
        return 0
    else
        echo -e "  ${COLOR_RED}✗${COLOR_RESET} $assert_desc (pattern not found)"
        TEST_MATRIX_FAILED_COUNT=$((TEST_MATRIX_FAILED_COUNT + 1))
        return 1
    fi
}

# Finish test matrix and print results
# Usage: finish_test_matrix "test suite name"
finish_test_matrix() {
    local test_name="$1"

    echo ""
    if [ "$TEST_MATRIX_FAILED_COUNT" -eq 0 ]; then
        echo -e "${COLOR_BOLD}${COLOR_GREEN}=== All $TEST_MATRIX_TEST_COUNT $test_name passed ===${COLOR_RESET}"
    else
        echo -e "${COLOR_BOLD}${COLOR_RED}=== $TEST_MATRIX_FAILED_COUNT of $TEST_MATRIX_TEST_COUNT $test_name failed ===${COLOR_RESET}"
        exit 1
    fi
}
