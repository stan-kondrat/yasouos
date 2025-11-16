#!/bin/bash

# Test arp-broadcast application - send ARP broadcast between two devices
# Usage: ./apps/arp-broadcast/arp_broadcast.test.sh [-v] [riscv|amd64|arm64]
#
# Examples:
#   ./apps/arp-broadcast/arp_broadcast.test.sh              # Run all architectures
#   ./apps/arp-broadcast/arp_broadcast.test.sh -v           # Run with verbose output
#   ./apps/arp-broadcast/arp_broadcast.test.sh riscv        # Run RISC-V only
#   ./apps/arp-broadcast/arp_broadcast.test.sh amd64        # Run AMD64 only

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$PROJECT_ROOT/tests/common.sh"

# Common test function for all architectures
run_arp_broadcast_test() {
    local arch=$1
    local boot_type=$2

    qemu_cmd=$(get_full_qemu_cmd "$arch" "$boot_type")

    # Create pcap file for each network device
    pcap_file0=$(mktemp)
    pcap_file1=$(mktemp)
    pcap_file2=$(mktemp)

    # Select virtio-net device based on architecture
    local virtio_device=$([ "$arch" = "amd64" ] && \
        echo "virtio-net-pci" || echo "virtio-net-device")

    qemu_args=(
        -append "'app=arp-broadcast'"
        -device "$virtio_device,netdev=net0,mac=52:54:00:12:34:56"
        -device "rtl8139,netdev=net1,mac=52:54:00:12:34:57"
        -device "e1000,netdev=net2,mac=52:54:00:12:34:58"
        -netdev hubport,id=net0,hubid=0
        -netdev hubport,id=net1,hubid=0
        -netdev hubport,id=net2,hubid=0
        -object "filter-dump,id=dump0,netdev=net0,file=$pcap_file0"
        -object "filter-dump,id=dump1,netdev=net1,file=$pcap_file1"
        -object "filter-dump,id=dump2,netdev=net2,file=$pcap_file2"
    )

    output=$(run_test_case "$qemu_cmd ${qemu_args[*]}")

    # Assert guest OS stdout - device initialization and MAC addresses

    assert_count "$output" "virtio-net@0.1.0] MAC: 52:54:00:12:34:56" 1 "virtio-net MAC correct"
    assert_count "$output" "rtl8139@0.1.0] MAC: 52:54:00:12:34:57" 1 "rtl8139 MAC correct"
    assert_count "$output" "e1000@0.1.0] MAC: 52:54:00:12:34:58" 1 "e1000 MAC correct"

    # Assert pcap files
    pcap_output0=$(tcpdump -qns 0 -r "$pcap_file0" 2>&1 || echo "")
    pcap_output1=$(tcpdump -qns 0 -r "$pcap_file1" 2>&1 || echo "")
    pcap_output2=$(tcpdump -qns 0 -r "$pcap_file2" 2>&1 || echo "")

    # Verify PCAP files contain ARP broadcast packets
    assert_contains "$pcap_output0" "link-type EN10MB" "Device 0 PCAP file readable"
    assert_contains "$pcap_output0" "ARP, Request who-has 10.0.2.15 tell 10.0.2.1" "Device 0 captured ARP request"

    assert_contains "$pcap_output1" "link-type EN10MB" "Device 1 PCAP file readable"
    assert_contains "$pcap_output1" "ARP, Request who-has 10.0.2.15 tell 10.0.2.1" "Device 1 captured ARP request"

    assert_contains "$pcap_output2" "link-type EN10MB" "Device 2 PCAP file readable"
    assert_contains "$pcap_output2" "ARP, Request who-has 10.0.2.15 tell 10.0.2.1" "Device 2 captured ARP request"

    if [ "$VERBOSE" = "1" ]; then
        echo "=== Full output ==="
        echo "$output"
        echo "==================="
        echo ""
        echo "=== PCAP device 0: $pcap_file0 ==="
        tcpdump -qns 0 -X -r "$pcap_file0" 2>&1 || true
        echo "==================="
        echo ""
        echo "=== PCAP device 1: $pcap_file1 ==="
        tcpdump -qns 0 -X -r "$pcap_file1" 2>&1 || true
        echo "==================="
        echo ""
        echo "=== PCAP device 2: $pcap_file2 ==="
        tcpdump -qns 0 -X -r "$pcap_file2" 2>&1 || true
        echo "==================="
    fi

    rm -f "$pcap_file0" "$pcap_file1" "$pcap_file2"
}

init_test_matrix "$@" "Testing arp-broadcast application"

for arch in $TEST_MATRIX_ARCH; do
    for boot_type in $TEST_MATRIX_BOOT_TYPE; do

        # raw image boot_type skipped
        if [ "$boot_type" = "image" ]; then
            continue
        fi

        # TODO: add support
        if [ "$arch" = "arm64" ]; then
            continue 
        fi

        test_section "arp-broadcast: $arch ($boot_type) with 3 network devices"
        run_arp_broadcast_test "$arch" "$boot_type"
    done
done

finish_test_matrix "arp-broadcast application tests"
