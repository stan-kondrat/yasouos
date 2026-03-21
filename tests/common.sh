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
# Supports comma-separated values like --arch=riscv,amd64
# Returns: Space-separated list of architectures
# On error, prints to stderr and returns non-zero exit code
parse_arch() {
    local arch_arg="$1"

    if [ -z "$arch_arg" ]; then
        echo "riscv arm64 amd64"
        return 0
    fi

    # Strip --arch= or ARCH= prefix if present
    local value="${arch_arg#--arch=}"
    value="${value#ARCH=}"

    local result=""
    IFS=',' read -ra arch_list <<< "$value"
    for arch in "${arch_list[@]}"; do
        # Trim whitespace
        arch=$(echo "$arch" | xargs)
        case "$arch" in
            riscv)
                result="$result riscv"
                ;;
            arm64)
                result="$result arm64"
                ;;
            amd64)
                result="$result amd64"
                ;;
            *)
                echo "Error: Invalid architecture '$arch'. Use --arch=riscv,arm64,amd64" >&2
                return 1
                ;;
        esac
    done

    # Trim leading space and print
    echo "$result" | xargs
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

# Test framework variables
TEST_MATRIX_ARCH=""
TEST_MATRIX_BOOT_TYPE=""
TEST_MATRIX_NET_DEVICE=""
TEST_MATRIX_VERBOSE=0
TEST_MATRIX_TEST_COUNT=0
TEST_MATRIX_FAILED_COUNT=0
TEST_MATRIX_TIMEOUT_SEC=5
COLOR_CYAN='\033[0;36m'

# Initialize test matrix framework
# Usage: init_test_matrix "$@" ["optional custom message"]
# Parses command line arguments for:
#   -v: verbose mode
#   --arch=riscv|arm64|amd64: specify architecture (default: all, comma-separated supported)
#   --boot=kernel|image|iso: specify boot type (default: all, comma-separated supported)
#   --netdev=e1000|rtl8139|virtio-net: specify network device (default: all, comma-separated supported)
# Optionally displays a custom message before parsing
init_test_matrix() {
    local message=""
    local args=("$@")

    # Check if last argument is a custom message (not a known flag)
    local last_arg="${args[${#args[@]}-1]}"
    if [ "${#args[@]}" -gt 0 ] && [ "$last_arg" != "-v" ] && \
       ! [[ "$last_arg" == --arch=* ]] && \
       ! [[ "$last_arg" == --boot=* ]] && \
       ! [[ "$last_arg" == --netdev=* ]]; then
        message="$last_arg"
        # Remove the message from args
        unset 'args[${#args[@]}-1]'
    fi

    eval "$(parse_verbose_flag "${args[@]}")"
    TEST_MATRIX_VERBOSE=$VERBOSE

    # Remove -v flag from args array if present
    local cleaned_args=()
    for arg in "${args[@]}"; do
        if [ "$arg" != "-v" ]; then
            cleaned_args+=("$arg")
        fi
    done

    # Parse explicit arguments and detect unknown ones
    local arch_arg=""
    local boot_type_arg=""
    local net_device_arg=""

    for arg in "${cleaned_args[@]}"; do
        case "$arg" in
            --arch=*)
                arch_arg="$arg"
                ;;
            --boot=*)
                boot_type_arg="$arg"
                ;;
            --netdev=*)
                net_device_arg="$arg"
                ;;
            *)
                # Any remaining unknown argument is an error
                echo "Error: Unknown argument '$arg'" >&2
                echo "Usage: [-v] [--arch=riscv,arm64,amd64] [--boot=kernel,image,iso] [--netdev=e1000,rtl8139,virtio-net]" >&2
                exit 1
                ;;
        esac
    done

    # Display custom message if provided
    if [ -n "$message" ]; then
        echo -e "${COLOR_CYAN}${message}${COLOR_RESET}"
    fi

    # Parse architecture (default: all architectures)
    if [ -z "$arch_arg" ]; then
        TEST_MATRIX_ARCH="amd64 arm64 riscv"
    else
        if ! TEST_MATRIX_ARCH=$(parse_arch "$arch_arg"); then
            exit 1
        fi
    fi

    # Parse boot type (default: all boot types)
    if [ -z "$boot_type_arg" ]; then
        TEST_MATRIX_BOOT_TYPE="kernel image iso"
    else
        # Strip --boot= prefix
        local boot_value="${boot_type_arg#--boot=}"
        local result=""
        local boot_error=0
        IFS=',' read -ra boot_list <<< "$boot_value"
        for boot in "${boot_list[@]}"; do
            # Trim whitespace
            boot=$(echo "$boot" | xargs)
            case "$boot" in
                kernel)
                    result="$result kernel"
                    ;;
                image)
                    result="$result image"
                    ;;
                iso)
                    result="$result iso"
                    ;;
                *)
                    echo "Error: Invalid boot type '$boot'. Use --boot=kernel,image,iso" >&2
                    boot_error=1
                    ;;
            esac
        done
        if [ $boot_error -eq 1 ]; then
            exit 1
        fi
        TEST_MATRIX_BOOT_TYPE=$(echo "$result" | xargs)
    fi

    # Store the network device argument for later use with get_net_devices_for_arch
    TEST_MATRIX_NET_DEVICE="$net_device_arg"

    # Register cleanup for any cmdline temp file created during tests
    trap "rm -f /tmp/yasouos-cmdline-$$.bin" EXIT
}

# Get network devices for a specific architecture
# Usage: get_net_devices_for_arch <arch> <net_device_arg>
# Returns: Space-separated list of network devices for the architecture
# Supports comma-separated values like --netdev=e1000,virtio-net
# On error, prints to stderr and returns non-zero exit code
get_net_devices_for_arch() {
    local arch="$1"
    local net_device_arg="$2"
    local virtio_device=$([ "$arch" = "amd64" ] && echo "virtio-net-pci" || echo "virtio-net-device")
    local all_devices="e1000 rtl8139 $virtio_device"

    # If empty or not specified, return all devices
    if [ -z "$net_device_arg" ]; then
        echo "$all_devices"
        return 0
    fi

    # Strip --netdev= prefix
    local value="${net_device_arg#--netdev=}"
    local result=""
    local dev_error=0

    IFS=',' read -ra dev_list <<< "$value"
    for dev in "${dev_list[@]}"; do
        # Trim whitespace
        dev=$(echo "$dev" | xargs)
        case "$dev" in
            e1000)
                result="$result e1000"
                ;;
            rtl8139)
                result="$result rtl8139"
                ;;
            virtio-net)
                result="$result $virtio_device"
                ;;
            *)
                echo "Error: Invalid network device '$dev'. Use --netdev=e1000,rtl8139,virtio-net" >&2
                dev_error=1
                ;;
        esac
    done

    if [ $dev_error -eq 1 ]; then
        return 1
    fi

    # Trim leading space and print, or return all if empty
    local trimmed_result=$(echo "$result" | xargs)
    if [ -z "$trimmed_result" ]; then
        echo "$all_devices"
    else
        echo "$trimmed_result"
    fi
}

# Build full QEMU command with machine flags, kernel/image, and common flags
# Usage: get_full_qemu_cmd <arch> <boot_type> [cmdline]
# Returns: Complete QEMU command string ready to execute
get_full_qemu_cmd() {
    local arch="$1"
    local boot_type="$2"
    local cmdline="$3"  # Optional: command line to pass

    local qemu_binary
    qemu_binary=$(get_qemu_cmd "$arch")

    local machine_flags
    machine_flags=$(get_qemu_machine_flags "$arch")

    local append_arg=""

    # Handle cmdline injection based on arch and boot type
    if [ -n "$cmdline" ]; then
        if [ "$arch" = "amd64" ] && [ "$boot_type" = "image" ]; then
            # AMD64 raw image boot: inject cmdline via QEMU loader device at 0x200000.
            # 0x200000 (2MB) survives the full boot: above BIOS/SeaBIOS range,
            # above kernel BSS/stack (~0x167000), and identity-mapped in long mode.
            local cmdline_file="/tmp/yasouos-cmdline-$$.bin"
            printf "%s\0" "$cmdline" > "$cmdline_file"
            append_arg="-device loader,file=$cmdline_file,addr=0x200000,force-raw=on"
        elif [ "$arch" = "amd64" ] && [ "$boot_type" = "kernel" ]; then
            # AMD64 kernel boot: use -append
            append_arg="-append '$cmdline'"
        elif [ "$arch" = "amd64" ] && [ "$boot_type" = "iso" ]; then
            # AMD64 ISO boot (PVH): use -append
            append_arg="-append '$cmdline'"
        else
            # RISC-V and ARM64: use -append
            append_arg="-append '$cmdline'"
        fi
    fi

    if [ "$boot_type" = "image" ]; then
        local disk
        disk=$(get_disk_path "$arch")
        if [ "$arch" = "amd64" ]; then
            echo "$qemu_binary $machine_flags -boot order=c -drive file=$disk,format=raw $append_arg -nographic --no-reboot"
        else
            # For RISC-V and ARM64, disk images need kernel flag (no bootloader yet)
            echo "$qemu_binary $machine_flags -kernel $disk $append_arg -nographic --no-reboot"
        fi
    else
        local kernel
        kernel=$(get_kernel_path "$arch")
        echo "$qemu_binary $machine_flags -kernel $kernel $append_arg -nographic --no-reboot"
    fi
}

# Run single test case and return output
# Usage: output=$(run_test_case "<qemu_command>" [timeout_sec])
# The function prints test metadata to stderr and returns test output to stdout
run_test_case() {
    local full_qemu_command="$1"
    local timeout_sec="${2:-$TEST_MATRIX_TIMEOUT_SEC}"

    test_timer_start

    local tmpfile
    tmpfile=$(mktemp)

    bash -c "$full_qemu_command" > "${tmpfile}" 2>&1 &
    local qemu_pid=$!

    # Wait for QEMU with timeout (in 100ms intervals)
    local count=0
    local max_count=$((timeout_sec * 10))
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
