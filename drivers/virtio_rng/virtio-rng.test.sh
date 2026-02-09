#!/bin/bash

# Test VirtIO RNG driver with hardware devices and software fallback
# Usage: ./drivers/virtio_rng/virtio-rng.test.sh [-v] [arch] [boot_type]
#   -v: verbose mode (prints QEMU output)
#   arch: riscv|arm64|amd64 (if not specified, runs all)
#   boot_type: kernel|image (only kernel is tested, image is ignored)
#
# Examples:
#   ./drivers/virtio_rng/virtio-rng.test.sh              # Run all tests
#   ./drivers/virtio_rng/virtio-rng.test.sh -v           # Run all tests with verbose output
#   ./drivers/virtio_rng/virtio-rng.test.sh amd64        # Run AMD64 tests (both pc and q35)
#   ./drivers/virtio_rng/virtio-rng.test.sh -v arm64     # Run ARM64 tests with verbose output

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$PROJECT_ROOT/tests/common.sh"

init_test_matrix "$@" "Testing VirtIO RNG driver"

# Hardware tests with two RNG devices
test_section "Hardware RNG Tests (two devices)"

for arch in $TEST_MATRIX_ARCH; do
    for boot_type in $TEST_MATRIX_BOOT_TYPE; do
        test_section "$arch $boot_type - Hardware RNG (two devices)"

        # AMD64 raw image doesn't support command line args (no bootloader yet)
        if [ "$arch" = "amd64" ] && [ "$boot_type" = "image" ]; then
            skip_test_case "AMD64 raw image doesn't support command line args"
            continue
        fi

        qemu_cmd=$(get_full_qemu_cmd "$arch" "$boot_type")

        # Build device arguments based on architecture
        if [ "$arch" = "amd64" ]; then
            # AMD64 uses virtio-rng-pci
            device_args="-object rng-random,id=rng0,filename=/dev/urandom -device virtio-rng-pci,rng=rng0 -object rng-random,id=rng1,filename=/dev/random -device virtio-rng-pci,rng=rng1"
        else
            # ARM64 and RISC-V use virtio-rng-device
            device_args="-object rng-random,id=rng0,filename=/dev/urandom -device virtio-rng-device,rng=rng0 -object rng-random,id=rng1,filename=/dev/random -device virtio-rng-device,rng=rng1"
        fi

        qemu_args=(
            -append "'log=debug app=random-hardware app=random-hardware'"
        )
        output=$(run_test_case "$qemu_cmd ${qemu_args[*]} $device_args")
        assert_count "$output" "Random (hardware):" 2 "Hardware RNG working (found 'Random (hardware):' twice)"
    done
done

# Software fallback tests with one device
test_section "Software Fallback Tests (one device)"

for arch in $TEST_MATRIX_ARCH; do
    for boot_type in $TEST_MATRIX_BOOT_TYPE; do
        test_section "$arch $boot_type - Software fallback (one device)"

        # Only kernel boot is supported (no image boot)
        if [ "$boot_type" = "image" ]; then
            skip_test_case "Only kernel boot supported"
            continue
        fi

        qemu_cmd=$(get_full_qemu_cmd "$arch" "$boot_type")

        # Build device arguments for ONE device only
        if [ "$arch" = "amd64" ]; then
            # AMD64 uses virtio-rng-pci
            device_args="-object rng-random,id=rng0,filename=/dev/urandom -device virtio-rng-pci,rng=rng0"
        else
            # ARM64 and RISC-V use virtio-rng-device
            device_args="-object rng-random,id=rng0,filename=/dev/urandom -device virtio-rng-device,rng=rng0"
        fi

        qemu_args=(
            -append "'log=debug app=random-hardware app=random-hardware'"
        )
        output=$(run_test_case "$qemu_cmd ${qemu_args[*]} $device_args")
        assert_count "$output" "Random (hardware):" 1 "Hardware RNG used once"
        assert_count "$output" "Random (software):" 1 "Software fallback used once"
    done
done

finish_test_matrix "VirtIO RNG tests"
