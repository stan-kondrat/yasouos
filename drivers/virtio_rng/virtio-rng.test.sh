#!/bin/bash

# Test VirtIO RNG driver with hardware devices and software fallback
# Usage: ./drivers/virtio_rng/virtio-rng.test.sh [-v] [arch] [boot_type]
#   -v: verbose mode (prints QEMU output)
#   arch: riscv|arm64|amd64 (if not specified, runs all)
#   boot_type: kernel|image (only kernel is tested, image is ignored)
#
# Examples:
#   ./drivers/virtio_rng/virtio-rng.test.sh              # Run all 8 tests
#   ./drivers/virtio_rng/virtio-rng.test.sh -v           # Run all tests with verbose output
#   ./drivers/virtio_rng/virtio-rng.test.sh amd64        # Run AMD64 tests (both pc and q35)
#   ./drivers/virtio_rng/virtio-rng.test.sh -v arm64     # Run ARM64 tests with verbose output
#
# Tests 8 configurations:
# 1-4: Hardware RNG with two devices (ARM64, RISC-V, AMD64-pc, AMD64-q35)
# 5-8: Software fallback with one devices (ARM64, RISC-V, AMD64-pc, AMD64-q35)

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
    echo "Skipping VirtIO RNG tests (only kernel boot supported)"
    exit 0
fi

TIMEOUT=5

# Color for info messages
COLOR_CYAN='\033[0;36m'
COLOR_RESET='\033[0m'

echo -e "${COLOR_CYAN}Testing VirtIO RNG driver${COLOR_RESET}"

# Run QEMU test with custom machine and device flags
run_qemu_rng_test() {
    local arch="$1"
    local image="$2"
    local machine_type="$3"
    local device_args="$4"
    local append_args="$5"
    local timeout_val="${6:-5}"

    local qemu_cmd
    qemu_cmd=$(get_qemu_cmd "$arch")

    # Create temporary file for output
    local tmpfile=$(mktemp)

    # Build command array
    local -a cmd=("${qemu_cmd}")

    # Add machine-specific flags
    if [ "$arch" = "amd64" ]; then
        cmd+=(
            -machine "$machine_type"
            -cpu qemu64
            -m 128M
            -device isa-debug-exit,iobase=0xf4,iosize=0x04
        )
    elif [ "$arch" = "arm64" ]; then
        cmd+=(
            -machine virt
            -cpu cortex-a53
        )
    elif [ "$arch" = "riscv" ]; then
        cmd+=(
            -machine virt
            -bios default
        )
    fi

    # Add kernel and basic flags
    cmd+=(
        -kernel "${image}"
        -append "${append_args}"
        -nographic
        --no-reboot
    )

    # Add device-specific arguments if provided
    if [ -n "$device_args" ]; then
        eval "cmd+=($device_args)"
    fi

    # Execute with timeout
    "${cmd[@]}" > "${tmpfile}" 2>&1 &
    local qemu_pid=$!

    local count=0
    local max_count=$((timeout_val * 10))
    while kill -0 "$qemu_pid" 2>/dev/null && [ $count -lt $max_count ]; do
        sleep 0.1
        count=$((count + 1))
    done

    if kill -0 "$qemu_pid" 2>/dev/null; then
        kill -TERM "$qemu_pid" 2>/dev/null || true
        sleep 0.2
        kill -KILL "$qemu_pid" 2>/dev/null || true
    fi

    wait "$qemu_pid" 2>/dev/null || true

    # Clean and return output
    sed 's/\x1b\[[0-9;?]*[mGKHJABCDsuhl]//g' "${tmpfile}" | \
    sed 's/\x1b[()][0AB]//g' | \
    sed 's/\x1b[=>]//g' | \
    tr -d '\000-\010\013\014\016-\037\177'

    rm -f "${tmpfile}"
}

run_hardware_test() {
    local test_num="$1"
    local arch="$2"
    local machine_type="$3"
    local desc="$4"

    echo "  Test $test_num: $desc - Hardware RNG (two devices)"

    local kernel
    kernel=$(get_kernel_path "$arch")
    check_image_exists "$kernel" "$arch" || return 1

    # Build device arguments based on architecture
    local device_args=""
    if [ "$arch" = "amd64" ]; then
        # AMD64 uses virtio-rng-pci
        device_args="-object rng-random,id=rng0,filename=/dev/urandom -device virtio-rng-pci,rng=rng0 -object rng-random,id=rng1,filename=/dev/random -device virtio-rng-pci,rng=rng1"
    else
        # ARM64 and RISC-V use virtio-rng-device
        device_args="-object rng-random,id=rng0,filename=/dev/urandom -device virtio-rng-device,rng=rng0 -object rng-random,id=rng1,filename=/dev/random -device virtio-rng-device,rng=rng1"
    fi

    test_timer_start
    output=$(run_qemu_rng_test "$arch" "$kernel" "$machine_type" "$device_args" "app=random-hardware app=random-hardware" "$TIMEOUT")

    if [ "$VERBOSE" -eq 1 ]; then
        echo "    --- QEMU Output ---"
        echo "$output" | sed 's/^/    /'
        echo "    --- End Output ---"
    fi

    # Check for successful hardware RNG output (should appear twice for two devices)
    local hw_count=$(echo "$output" | grep -c "Random (hardware):" || true)
    local sw_count=$(echo "$output" | grep -c "Random (software):" || true)

    if [ "$hw_count" -eq 2 ] && [ "$sw_count" -eq 0 ]; then
        test_pass "Hardware RNG working (found 'Random (hardware):' twice)"
        return 0
    else
        test_fail "Expected 2 hardware, 0 software (found $hw_count hardware, $sw_count software)"
        return 1
    fi
}

run_software_test() {
    local test_num="$1"
    local arch="$2"
    local machine_type="$3"
    local desc="$4"

    echo "  Test $test_num: $desc - Software fallback (one device)"

    local kernel
    kernel=$(get_kernel_path "$arch")
    check_image_exists "$kernel" "$arch" || return 1

    # Build device arguments for ONE device only
    local device_args=""
    if [ "$arch" = "amd64" ]; then
        # AMD64 uses virtio-rng-pci
        device_args="-object rng-random,id=rng0,filename=/dev/urandom -device virtio-rng-pci,rng=rng0"
    else
        # ARM64 and RISC-V use virtio-rng-device
        device_args="-object rng-random,id=rng0,filename=/dev/urandom -device virtio-rng-device,rng=rng0"
    fi

    test_timer_start
    output=$(run_qemu_rng_test "$arch" "$kernel" "$machine_type" "$device_args" "app=random-hardware app=random-hardware" "$TIMEOUT")

    if [ "$VERBOSE" -eq 1 ]; then
        echo "    --- QEMU Output ---"
        echo "$output" | sed 's/^/    /'
        echo "    --- End Output ---"
    fi

    # Check output: first call uses hardware, second call uses software fallback
    local hw_count=$(echo "$output" | grep -c "Random (hardware):" || true)
    local sw_count=$(echo "$output" | grep -c "Random (software):" || true)

    if [ "$hw_count" -eq 1 ] && [ "$sw_count" -eq 1 ]; then
        test_pass "Hardware RNG used once, software fallback once"
        return 0
    else
        test_fail "Expected 1 hardware, 1 software (found $hw_count hardware, $sw_count software)"
        return 1
    fi
}

test_count=0
failed_count=0

# Hardware tests with two RNG devices
test_section "Hardware RNG Tests (two devices)"

for arch in $ARCHS; do
    case "$arch" in
        arm64)
            test_count=$((test_count + 1))
            run_hardware_test "$test_count" "arm64" "virt" "ARM64" || failed_count=$((failed_count + 1))
            ;;
        riscv)
            test_count=$((test_count + 1))
            run_hardware_test "$test_count" "riscv" "virt" "RISC-V" || failed_count=$((failed_count + 1))
            ;;
        amd64)
            test_count=$((test_count + 1))
            run_hardware_test "$test_count" "amd64" "pc" "AMD64-pc" || failed_count=$((failed_count + 1))

            test_count=$((test_count + 1))
            run_hardware_test "$test_count" "amd64" "q35" "AMD64-q35" || failed_count=$((failed_count + 1))
            ;;
    esac
done

# Software fallback tests with one device
test_section "Software Fallback Tests (one device)"

for arch in $ARCHS; do
    case "$arch" in
        arm64)
            test_count=$((test_count + 1))
            run_software_test "$test_count" "arm64" "virt" "ARM64" || failed_count=$((failed_count + 1))
            ;;
        riscv)
            test_count=$((test_count + 1))
            run_software_test "$test_count" "riscv" "virt" "RISC-V" || failed_count=$((failed_count + 1))
            ;;
        amd64)
            test_count=$((test_count + 1))
            run_software_test "$test_count" "amd64" "pc" "AMD64-pc" || failed_count=$((failed_count + 1))

            test_count=$((test_count + 1))
            run_software_test "$test_count" "amd64" "q35" "AMD64-q35" || failed_count=$((failed_count + 1))
            ;;
    esac
done

echo ""
if [ $failed_count -eq 0 ]; then
    echo -e "${COLOR_BOLD}${COLOR_GREEN}=== All $test_count VirtIO RNG tests passed ===${COLOR_RESET}"
else
    echo -e "${COLOR_BOLD}${COLOR_RED}=== $failed_count of $test_count VirtIO RNG tests failed ===${COLOR_RESET}"
    exit 1
fi
