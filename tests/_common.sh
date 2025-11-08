#!/bin/bash

# Common test utilities for YasouOS tests

set -e

# Parse verbose flag and detect CI environment
# Usage: eval "$(parse_verbose_flag "$@")"
# Outputs: VERBOSE=1 and optional shift command
parse_verbose_flag() {
    # Auto-enable in CI environments
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

# Run QEMU with timeout and capture output
run_qemu_test() {
    local arch="$1"
    local image="$2"
    local append_args="$3"
    local timeout_val="${4:-3}"

    local qemu_cmd
    qemu_cmd=$(get_qemu_cmd "$arch")

    local machine_flags
    machine_flags=$(get_qemu_machine_flags "$arch")

    # Create temporary file for output
    local tmpfile=$(mktemp)

    # Build command array for proper quoting
    local -a cmd=(
        "${qemu_cmd}"
    )

    # Add machine flags (word splitting intentional here)
    for flag in ${machine_flags}; do
        cmd+=("${flag}")
    done

    # Check if this is a disk image or kernel
    if [[ "$image" == *.img ]]; then
        # Disk image boot - only AMD64 has bootloader support
        if [ "$arch" = "amd64" ]; then
            cmd+=(
                -drive "file=${image},format=raw"
                -nographic
                --no-reboot
            )
        else
            # For RISC-V and ARM64, disk images need kernel flag (no bootloader yet)
            cmd+=(
                -kernel "${image}"
                -append "${append_args}"
                -nographic
                --no-reboot
            )
        fi
    else
        # Kernel boot
        cmd+=(
            -kernel "${image}"
            -append "${append_args}"
            -nographic
            --no-reboot
        )
    fi

    # Execute with timeout using background process
    "${cmd[@]}" > "${tmpfile}" 2>&1 &
    local qemu_pid=$!

    # Wait for process with timeout (in deciseconds, 10 deciseconds = 1 second)
    local count=0
    local max_count=$((timeout_val * 10))
    while kill -0 "$qemu_pid" 2>/dev/null && [ $count -lt $max_count ]; do
        sleep 0.1
        count=$((count + 1))
    done

    # Kill process if still running
    if kill -0 "$qemu_pid" 2>/dev/null; then
        kill -TERM "$qemu_pid" 2>/dev/null || true
        sleep 0.2
        kill -KILL "$qemu_pid" 2>/dev/null || true
    fi

    # Wait for process to terminate
    wait "$qemu_pid" 2>/dev/null || true

    # Read the output from temp file and strip ANSI/control sequences
    # Remove ESC sequences and control chars, preserving newlines and tabs
    sed 's/\x1b\[[0-9;?]*[mGKHJABCDsuhl]//g' "${tmpfile}" | \
    sed 's/\x1b[()][0AB]//g' | \
    sed 's/\x1b[=>]//g' | \
    tr -d '\000-\010\013\014\016-\037\177'

    # Clean up
    rm -f "${tmpfile}"
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
    # Use GNU date on Linux, gdate on macOS if available, otherwise seconds
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

# Test result helpers
test_pass() {
    local elapsed=$(test_timer_stop)
    local time_str=$(format_time $elapsed)
    echo -e "  ${COLOR_GREEN}✓${COLOR_RESET} $1 ${COLOR_GRAY}(${time_str})${COLOR_RESET}"
}

test_fail() {
    local elapsed=$(test_timer_stop)
    local time_str=$(format_time $elapsed)
    echo -e "  ${COLOR_RED}✗${COLOR_RESET} $1 ${COLOR_GRAY}(${time_str})${COLOR_RESET}"
}

test_section() {
    echo ""
    echo -e "${COLOR_BOLD}${COLOR_BLUE}=== $1 ===${COLOR_RESET}"
}

# Check if image file exists
check_image_exists() {
    local image="$1"
    local arch="$2"

    if [ ! -f "$image" ]; then
        test_fail "$image not found (run 'make ARCH=$arch' first)"
        return 1
    fi
    return 0
}
