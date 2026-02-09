#!/bin/bash

# Test packet-print application - prints UDP packets
# Usage: ./apps/packet-print/packet_print_udp.test.sh [-v] [riscv|amd64] [e1000|rtl8139|virtio-net]
#
# Examples:
#   ./apps/packet-print/packet_print_udp.test.sh              # Run all architectures
#   ./apps/packet-print/packet_print_udp.test.sh -v           # Run with verbose output
#   ./apps/packet-print/packet_print_udp.test.sh riscv        # Run RISC-V only
#   ./apps/packet-print/packet_print_udp.test.sh amd64        # Run AMD64 only

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$PROJECT_ROOT/tests/common.sh"

# Common test function for all architectures
run_packet_print_test() {
    local arch=$1
    local boot_type=$2
    local net_device=$3
    local random_num=$((1 + RANDOM % 8))
    local request_string="ping-$random_num"
    local response_expected="pong-$((random_num + 1))"
    local udp_port_guest=5000
    local udp_port_host=$((20000 + RANDOM % 10000))
    local udp_port_remote=$((10000 + RANDOM % 10000))

    qemu_cmd=$(get_full_qemu_cmd "$arch" "$boot_type")

    # Use dgram networking for direct UDP packet injection
    pcap_file=$(mktemp)
    qemu_args=(
        -append "'log=debug app=packet-print'"
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

    # Wait for QEMU to boot and app to start (check for "Listening for UDP packets")
    for i in {1..20}; do
        if grep -q "Listening for UDP packets" "$qemu_output"; then
            break
        fi
        sleep 0.2
    done

    # Wait for driver to be fully initialized (MAC address printed)
    for i in {1..10}; do
        if grep -q "MAC: 52:54:00:12:34:56" "$qemu_output"; then
            break
        fi
        sleep 0.2
    done

    # Send raw Ethernet frame with UDP packet
    if [ "$VERBOSE" = "1" ]; then
        echo "Sending UDP packet with $request_string via dgram socket to localhost:$udp_port_host..."
    fi

    # Create a raw Ethernet frame containing UDP packet using printf
    # Ethernet: dst=52:54:00:12:34:56, src=52:55:0a:00:02:02, type=0x0800 (IPv4)
    # IPv4: src=10.0.2.2, dst=10.0.2.15, proto=17 (UDP)
    # UDP: src=$udp_port_host, dst=$udp_port_guest, payload="ping-$random_num"
    packet_file=$(mktemp)

    # Build payload as ASCII string "ping-N"
    payload="$request_string"
    payload_len=${#payload}
    udp_len=$((8 + payload_len))
    ip_len=$((20 + udp_len))

    # Ethernet header (14 bytes)
    printf '\x52\x54\x00\x12\x34\x56' > "$packet_file"  # dst MAC
    printf '\x52\x55\x0a\x00\x02\x02' >> "$packet_file" # src MAC
    printf '\x08\x00' >> "$packet_file"                 # ethertype (IPv4)

    # IPv4 header (20 bytes)
    printf '\x45\x00' >> "$packet_file"                 # version+IHL, TOS
    printf "\\x$(printf '%02x' $((ip_len >> 8)))" >> "$packet_file"  # total length high byte
    printf "\\x$(printf '%02x' $((ip_len & 0xFF)))" >> "$packet_file"  # total length low byte
    printf '\x00\x01\x00\x00' >> "$packet_file"         # ID, flags+fragment
    printf '\x40\x11' >> "$packet_file"                 # TTL (64), protocol (UDP=17)
    printf '\x00\x00' >> "$packet_file"                 # checksum (0)
    printf '\x0a\x00\x02\x02' >> "$packet_file"         # src IP (10.0.2.2)
    printf '\x0a\x00\x02\x0f' >> "$packet_file"         # dst IP (10.0.2.15)

    # UDP header (8 bytes)
    printf "\\x$(printf '%02x' $((udp_port_host >> 8)))" >> "$packet_file"  # src port high byte
    printf "\\x$(printf '%02x' $((udp_port_host & 0xFF)))" >> "$packet_file"  # src port low byte
    printf "\\x$(printf '%02x' $((udp_port_guest >> 8)))" >> "$packet_file"  # dst port high byte
    printf "\\x$(printf '%02x' $((udp_port_guest & 0xFF)))" >> "$packet_file"  # dst port low byte
    printf "\\x$(printf '%02x' $((udp_len >> 8)))" >> "$packet_file"  # UDP length high byte
    printf "\\x$(printf '%02x' $((udp_len & 0xFF)))" >> "$packet_file"  # UDP length low byte
    printf '\x00\x00' >> "$packet_file"                 # checksum (0)

    # UDP payload: "ping-$random_num" as ASCII
    printf "%s" "$payload" >> "$packet_file"

    # Send packet via netcat
    timeout 1 nc -u -w 1 127.0.0.1 "$udp_port_host" < "$packet_file" 2>/dev/null || true

    rm -f "$packet_file"

    # Wait for packet processing (check for echo reply or system halt)
    for i in {1..20}; do
        if grep -q "Sent UDP echo reply" "$qemu_output" || grep -q "System halted" "$qemu_output"; then
            sleep 0.1  # Brief grace period for final output
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

    # Verify packet-print-udp app started
    assert_contains "$output" "Listening for UDP packets" "App started listening for UDP"

    # Verify MAC address was printed
    assert_contains "$output" "52:54:00:12:34:56" "Correct MAC address displayed"

    # Verify received ping message contains request_string in stdout
    assert_contains "$output" "$request_string" "Received $request_string in stdout"

    # Verify sent echo reply
    assert_contains "$output" "Sent UDP echo reply" "Sent UDP echo reply"

    # Check PCAP file for actual network traffic
    pcap_output=$(tcpdump -qns 0 -r "$pcap_file" 2>&1 || echo "")

    # Verify IPv4 packets in PCAP
    assert_contains "$pcap_output" "IP " "IPv4 packets in PCAP"

    # Verify UDP packets in PCAP (both request and response)
    assert_count "$pcap_output" "UDP" 2 "UDP echo (request and response in PCAP)"

    # Verify the echo response contains expected response in PCAP
    if [ "$VERBOSE" = "1" ]; then
        echo "Expected response: $response_expected (request: $request_string)"
    fi

    # Check PCAP for the response payload containing expected response
    pcap_ascii=$(tcpdump -qns 0 -A -r "$pcap_file" 2>&1 | grep -A 5 "10.0.2.15.$udp_port_guest > 10.0.2.2.$udp_port_host" || echo "")
    assert_contains "$pcap_ascii" "$response_expected" "UDP echo response payload verified ($response_expected in PCAP)"

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

init_test_matrix "$@" "Testing packet-print application (UDP)"

for arch in $TEST_MATRIX_ARCH; do
    for boot_type in $TEST_MATRIX_BOOT_TYPE; do

        #  raw image boot_type skipped
        if [ "$boot_type" = "image" ]; then
            continue
        fi

        # Get network devices for this specific architecture
        net_devices=$(get_net_devices_for_arch "$arch" "$TEST_MATRIX_NET_DEVICE")

        for device in $net_devices; do
            test_section "packet-print: UDP traffic $arch ($boot_type) with $device"
            run_packet_print_test "$arch" "$boot_type" "$device"
        done
    done
done

finish_test_matrix "packet-print application tests (UDP)"
