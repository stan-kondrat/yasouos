#!/bin/bash

# Benchmark HTTP Hello World application using autocannon
# Usage: ./benchmark/http-hello.sh [arch] [net-device]
#
# Examples:
#   ./benchmark/http-hello.sh              # Run all architectures
#   ./benchmark/http-hello.sh arm64        # Run ARM64 only
#   ./benchmark/http-hello.sh amd64 e1000  # Run AMD64 with e1000 only

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
source "$PROJECT_ROOT/tests/common.sh"

# Check autocannon is installed
if ! command -v autocannon >/dev/null 2>&1; then
    echo -e "${COLOR_RED}Error: autocannon is not installed.${COLOR_RESET}"
    echo "Install it with: npm install -g autocannon"
    exit 1
fi

# Parse arguments
ARCH_FILTER=$(parse_arch "$1")
NET_DEVICE_FILTER="$2"

for arch in $ARCH_FILTER; do
    net_devices=$(get_net_devices_for_arch "$arch" "$NET_DEVICE_FILTER")

    for device in $net_devices; do
        # Skip rtl8139 — QEMU TCP bug
        if [ "$device" = "rtl8139" ]; then
            echo -e "${COLOR_YELLOW}Skipping $arch / $device (QEMU TCP bug)${COLOR_RESET}"
            echo ""
            continue
        fi

        echo -e "${COLOR_BOLD}${COLOR_BLUE}=== Benchmark: $arch / $device ===${COLOR_RESET}"

        http_port_host=$((20000 + RANDOM % 10000))
        qemu_output=$(mktemp)

        qemu_binary=$(get_qemu_cmd "$arch")
        machine_flags=$(get_qemu_machine_flags "$arch")
        kernel_path=$(get_kernel_path "$arch")

        echo "Starting QEMU on port $http_port_host..."

        # Run QEMU directly (no bash -c wrapper) — machine_flags is intentionally unquoted
        # shellcheck disable=SC2086
        $qemu_binary $machine_flags \
            -kernel "$kernel_path" \
            -append "app=http-hello log=warn log.http-hello=info" \
            -device "$device,netdev=net0,mac=52:54:00:12:34:56" \
            -netdev "user,id=net0,hostfwd=tcp::${http_port_host}-:80" \
            -nographic --no-reboot > "$qemu_output" 2>&1 &
        qemu_pid=$!

        # Wait for app to start listening
        ready=0
        for i in $(seq 1 30); do
            if ! kill -0 "$qemu_pid" 2>/dev/null; then
                echo -e "${COLOR_RED}QEMU exited early:${COLOR_RESET}"
                cat "$qemu_output"
                break
            fi
            if grep -q "Listening on port" "$qemu_output" 2>/dev/null; then
                ready=1
                break
            fi
            sleep 0.2
        done

        if [ "$ready" -eq 0 ]; then
            echo -e "${COLOR_RED}Timed out waiting for QEMU to start. Output:${COLOR_RESET}"
            tail -5 "$qemu_output"
            kill "$qemu_pid" 2>/dev/null || true
            wait "$qemu_pid" 2>/dev/null || true
            rm -f "$qemu_output"
            echo ""
            continue
        fi

        echo -e "${COLOR_GREEN}QEMU ready, running autocannon...${COLOR_RESET}"
        echo ""

        autocannon -m GET -c 5 -d 20 -p 2 -w 1 "http://localhost:${http_port_host}/"
        # autocannon -m GET -c 100 -d 30 -p 10 -w 5 "http://localhost:${http_port_host}/"

        # Clean up
        kill "$qemu_pid" 2>/dev/null || true
        wait "$qemu_pid" 2>/dev/null || true
        rm -f "$qemu_output"

        echo ""
        echo -e "${COLOR_GRAY}────────────────────────────────────────${COLOR_RESET}"
        echo ""
    done
done

echo -e "${COLOR_BOLD}${COLOR_GREEN}Benchmark complete.${COLOR_RESET}"
