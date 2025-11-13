#!/bin/bash

# Run all YasouOS tests
# Usage: ./tests/run-all.sh [-v] [arch] [boot_type]
#   -v: verbose mode
#   arch: riscv|arm64|amd64 (if not specified, runs all)
#   boot_type: kernel|image (if not specified, runs both)

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Color codes
COLOR_RESET='\033[0m'
COLOR_GREEN='\033[0;32m'
COLOR_RED='\033[0;31m'
COLOR_BOLD='\033[1m'

# Discover all *.test.sh files recursively in the project
test_scripts=()
while IFS= read -r -d '' script; do
    test_scripts+=("$script")
done < <(find "$PROJECT_ROOT" -type f -name "*.test.sh" -print0)

# Sort test scripts for consistent execution order
IFS=$'\n' test_scripts=($(sort <<<"${test_scripts[*]}"))
unset IFS

# Run each test script
total_suites=0
total_failures=0
for script in "${test_scripts[@]}"; do
    echo -e "=============================="
    echo -e "$script" "$@"
    echo -e "=============================="
    "$script" "$@"
    result=$?
    total_suites=$((total_suites + 1))
    if [ $result -ne 0 ]; then
        total_failures=$((total_failures + 1))
    fi
    echo ""
done

# Display overall result
if [ $total_failures -eq 0 ]; then
    echo -e "${COLOR_BOLD}${COLOR_GREEN}=== All $total_suites test suite(s) passed ===${COLOR_RESET}"
    exit 0
else
    echo -e "${COLOR_BOLD}${COLOR_RED}=== $total_failures of $total_suites test suite(s) failed ===${COLOR_RESET}"
    exit 1
fi
