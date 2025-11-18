#!/bin/bash

# Test packet-print application - prints ICMP packets
# Usage: ./apps/packet-print/packet_print_icmp.test.sh [-v] [riscv|amd64] [e1000|rtl8139|virtio-net]
#
# Examples:
#   ./apps/packet-print/packet_print_icmp.test.sh              # Run all architectures
#   ./apps/packet-print/packet_print_icmp.test.sh -v           # Run with verbose output
#   ./apps/packet-print/packet_print_icmp.test.sh riscv        # Run RISC-V only
#   ./apps/packet-print/packet_print_icmp.test.sh amd64        # Run AMD64 only

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$PROJECT_ROOT/tests/common.sh"

# Common test function for all architectures
run_packet_print_icmp_test() {
    local arch=$1
    local boot_type=$2
    local net_device=$3

    qemu_cmd=$(get_full_qemu_cmd "$arch" "$boot_type")

    # Use dgram networking for ICMP testing (same as UDP test)
    pcap_file=$(mktemp)
    local udp_port_host=$((20000 + RANDOM % 10000))
    local udp_port_remote=$((10000 + RANDOM % 10000))

    qemu_args=(
        -append "'app=packet-print'"
        -device "$net_device,netdev=net0,mac=52:54:00:12:34:56"
        -netdev "dgram,id=net0,local.type=inet,local.host=127.0.0.1,local.port=$udp_port_host,remote.type=inet,remote.host=127.0.0.1,remote.port=$udp_port_remote"
        -object "filter-dump,id=dump,netdev=net0,file=$pcap_file"
    )

    # Create output file for QEMU
    qemu_output=$(mktemp)

    # Run QEMU in background
    full_cmd="$qemu_cmd ${qemu_args[*]} -nographic --no-reboot"

    if [ "$VERBOSE" = "1" ]; then
        echo "Running QEMU in background: $full_cmd"
    fi

    # Start QEMU in background and redirect output
    bash -c "$full_cmd" > "$qemu_output" 2>&1 &
    qemu_pid=$!

    # Wait for driver to be fully initialized (MAC address printed)
    for i in {1..10}; do
        if grep -q "MAC: 52:54:00:12:34:56" "$qemu_output"; then
            break
        fi
        sleep 0.2
    done

    # Send ICMP echo request packet via dgram
    if [ "$VERBOSE" = "1" ]; then
        echo "Sending ICMP echo request via dgram socket to localhost:$udp_port_host..."
    fi

    # Build raw ICMP echo request packet
    # Ethernet: dst=52:54:00:12:34:56, src=52:55:0a:00:02:02, type=0x0800 (IPv4)
    # IPv4: src=10.0.2.2, dst=10.0.2.15, protocol=1 (ICMP)
    # ICMP: type=8 (echo request), code=0, id=1, seq=1
    packet_file=$(mktemp)

    # Ethernet header (14 bytes)
    printf '\x52\x54\x00\x12\x34\x56' > "$packet_file"  # dst MAC
    printf '\x52\x55\x0a\x00\x02\x02' >> "$packet_file" # src MAC
    printf '\x08\x00' >> "$packet_file"                 # ethertype (IPv4)

    # IPv4 header (20 bytes)
    # Total length: 20 (IP header) + 8 (ICMP header) = 28 bytes
    printf '\x45\x00' >> "$packet_file"                 # version+IHL, TOS
    printf '\x00\x1c' >> "$packet_file"                 # total length (28)
    printf '\x00\x01\x00\x00' >> "$packet_file"         # ID, flags+fragment
    printf '\x40\x01' >> "$packet_file"                 # TTL (64), protocol (ICMP=1)
    printf '\xb6\xea' >> "$packet_file"                 # checksum (recalculated for src=10.0.2.2)
    printf '\x0a\x00\x02\x02' >> "$packet_file"         # src IP (10.0.2.2)
    printf '\x0a\x00\x02\x0f' >> "$packet_file"         # dst IP (10.0.2.15)

    # ICMP header (8 bytes)
    printf '\x08\x00' >> "$packet_file"                 # type (8=echo request), code (0)
    printf '\xf7\xff' >> "$packet_file"                 # checksum (calculated: 0xf7ff)
    printf '\x00\x01' >> "$packet_file"                 # id (1)
    printf '\x00\x01' >> "$packet_file"                 # sequence (1)

    # Send packet via netcat to dgram port
    timeout 1 nc -u -w 1 127.0.0.1 "$udp_port_host" < "$packet_file" 2>/dev/null || true

    rm -f "$packet_file"

    # Wait for packet processing
    for i in {1..30}; do
        if grep -q "\[ICMP\]" "$qemu_output" || grep -q "System halted" "$qemu_output"; then
            sleep 0.2
            break
        fi
        sleep 0.2
    done

    # Kill QEMU
    kill $qemu_pid 2>/dev/null || true
    wait $qemu_pid 2>/dev/null || true

    # Read output
    output=$(cat "$qemu_output")
    rm -f "$qemu_output"

    # Verify MAC address was printed
    assert_contains "$output" "52:54:00:12:34:56" "Correct MAC address displayed"

    # Verify ICMP packet was printed
    assert_contains "$output" "\[ICMP\]" "ICMP packet printed in stdout"

    # Verify ICMP echo reply was sent
    assert_contains "$output" "Sent ICMP echo reply" "Sent ICMP echo reply"

    # Check PCAP file for actual network traffic
    pcap_output=$(tcpdump -qns 0 -r "$pcap_file" 2>&1 || echo "")

    # Verify IPv4 packets in PCAP
    assert_contains "$pcap_output" "IP " "IPv4 packets in PCAP"

    # Verify ICMP packets in PCAP (both request and reply)
    assert_count "$pcap_output" "ICMP" 2 "ICMP echo (request and reply in PCAP)"

    if [ "$VERBOSE" = "1" ]; then
        echo "=== Full output ==="
        echo "$output"
        echo "==================="
        echo ""
        echo "=== PCAP file: $pcap_file ==="
        tcpdump -qns 0 -X -r "$pcap_file" 2>&1 || true
        echo "==================="
    fi

    rm -f "$pcap_file"
}

init_test_matrix "$@" "Testing packet-print application (ICMP)"

for arch in $TEST_MATRIX_ARCH; do
    for boot_type in $TEST_MATRIX_BOOT_TYPE; do

        #  raw image boot_type skipped
        if [ "$boot_type" = "image" ]; then
            continue
        fi

        # Get network devices for this specific architecture
        net_devices=$(get_net_devices_for_arch "$arch" "$TEST_MATRIX_NET_DEVICE")

        for device in $net_devices; do
            test_section "packet-print: ICMP traffic $arch ($boot_type) with $device"
            run_packet_print_icmp_test "$arch" "$boot_type" "$device"
        done
    done
done

finish_test_matrix "packet-print application tests (ICMP)"
