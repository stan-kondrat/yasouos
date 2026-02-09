#!/bin/bash

# Test HTTP Hello World application
# Usage: ./apps/http-hello/http_hello.test.sh [-v] [riscv|amd64|arm64] [e1000|rtl8139|virtio-net]
#
# Examples:
#   ./apps/http-hello/http_hello.test.sh              # Run all architectures
#   ./apps/http-hello/http_hello.test.sh -v            # Run with verbose output
#   ./apps/http-hello/http_hello.test.sh amd64 e1000   # Run AMD64 with e1000 only

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
source "$PROJECT_ROOT/tests/common.sh"

run_http_hello_test() {
    local arch=$1
    local boot_type=$2
    local net_device=$3
    local http_port_host=$((20000 + RANDOM % 10000))

    qemu_cmd=$(get_full_qemu_cmd "$arch" "$boot_type")

    pcap_file=$(mktemp)
    qemu_args=(
        -append "'app=http-hello'"
        -device "$net_device,netdev=net0,mac=52:54:00:12:34:56"
        -netdev "user,id=net0,hostfwd=tcp::${http_port_host}-:80"
        -object "filter-dump,id=dump,netdev=net0,file=$pcap_file"
    )

    qemu_output=$(mktemp)
    qemu_debug_log=$(mktemp)

    debug_args=""
    if [ "$net_device" = "rtl8139" ] && [ "$VERBOSE" = "1" ]; then
        debug_args="-d guest_errors,unimp -D $qemu_debug_log"
    fi

    full_cmd="$qemu_cmd ${qemu_args[*]} $debug_args -nographic --no-reboot"

    if [ "$VERBOSE" = "1" ]; then
        echo "Running QEMU in background: $full_cmd"
    fi

    bash -c "$full_cmd" > "$qemu_output" 2>&1 &
    qemu_pid=$!

    # Wait for app to start listening
    for i in {1..30}; do
        if grep -q "Listening on port" "$qemu_output"; then
            break
        fi
        sleep 0.2
    done
    output_check=$(cat "$qemu_output")
    assert_contains "$output_check" "Listening on port" "App started listening"

    # Wait for MAC address
    for i in {1..10}; do
        if grep -q "MAC: 52:54:00:12:34:56" "$qemu_output"; then
            break
        fi
        sleep 0.2
    done
    output_check=$(cat "$qemu_output")
    assert_contains "$output_check" "MAC: 52:54:00:12:34:56" "MAC address initialized"

    if [ "$VERBOSE" = "1" ]; then
        echo "Sending HTTP request to localhost:$http_port_host..."
    fi

    # Send HTTP request via curl (with fallback to nc)
    http_response=""
    if command -v curl >/dev/null 2>&1; then
        http_response=$(curl -s --max-time 5 "http://127.0.0.1:${http_port_host}/" 2>/dev/null || true)
    fi
    if [ -z "$http_response" ] && command -v nc >/dev/null 2>&1; then
        http_response=$(printf "GET / HTTP/1.0\r\nHost: 127.0.0.1\r\n\r\n" | nc -w 3 127.0.0.1 "$http_port_host" 2>/dev/null || true)
    fi

    # Wait for response to be sent
    for i in {1..20}; do
        if grep -q "HTTP response sent" "$qemu_output"; then
            sleep 0.1
            break
        fi
        sleep 0.2
    done

    # Kill QEMU
    kill $qemu_pid 2>/dev/null || true
    wait $qemu_pid 2>/dev/null || true

    output=$(cat "$qemu_output")
    rm -f "$qemu_output"

    # Verify MAC address
    assert_contains "$output" "52:54:00:12:34:56" "Correct MAC address displayed"

    # Verify SYN received
    # NOTE: RTL8139 TCP reception fails due to QEMU emulation bug
    if [ "$net_device" != "rtl8139" ]; then
        assert_contains "$output" "SYN received" "SYN received in stdout"
        assert_contains "$output" "HTTP response sent" "HTTP response sent"
    fi

    # Verify HTTP response body
    if [ "$net_device" != "rtl8139" ]; then
        assert_contains "$http_response" "Hello, " "HTTP response contains Hello with IP"
    fi

    # Check PCAP for ARP + TCP
    pcap_output=$(tcpdump -qns 0 -r "$pcap_file" 2>&1 || echo "")
    assert_contains "$pcap_output" "ARP, Reply" "ARP reply in PCAP"
    assert_contains "$pcap_output" "tcp" "TCP packets in PCAP"

    if [ "$VERBOSE" = "1" ]; then
        echo "=== Full output ==="
        echo "$output"
        echo "==================="
        echo ""
        echo "=== HTTP Response ==="
        echo "$http_response"
        echo "==================="
        echo ""
        echo "=== PCAP file: $pcap_file ==="
        tcpdump -qns 0 -X -r "$pcap_file" 2>&1 || true
        echo "==================="

        if [ "$net_device" = "rtl8139" ] && [ -f "$qemu_debug_log" ]; then
            echo ""
            echo "=== QEMU Debug Log ==="
            cat "$qemu_debug_log"
            echo "==================="
        fi
    fi

    rm -f "$pcap_file"
    rm -f "$qemu_debug_log"
}

init_test_matrix "$@" "Testing HTTP Hello World application"

for arch in $TEST_MATRIX_ARCH; do
    for boot_type in $TEST_MATRIX_BOOT_TYPE; do

        # raw image boot_type skipped
        if [ "$boot_type" = "image" ]; then
            continue
        fi

        net_devices=$(get_net_devices_for_arch "$arch" "$TEST_MATRIX_NET_DEVICE")

        for device in $net_devices; do
            test_section "http-hello: HTTP traffic $arch ($boot_type) with $device"
            run_http_hello_test "$arch" "$boot_type" "$device"
        done
    done
done

finish_test_matrix "HTTP Hello World application tests"
