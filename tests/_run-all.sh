#!/bin/bash

# Run all YasouOS tests
# Usage: ./tests/_run-all.sh [-v] [arch]

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Color codes
COLOR_RESET='\033[0m'
COLOR_GREEN='\033[0;32m'
COLOR_RED='\033[0;31m'
COLOR_BOLD='\033[1m'

# Discover all test scripts (excluding _common.sh and _run-all.sh)
test_scripts=()
for script in "$SCRIPT_DIR"/*.sh; do
    basename=$(basename "$script")
    # Skip utility files that start with underscore
    if [[ ! "$basename" =~ ^_ ]]; then
        test_scripts+=("$script")
    fi
done

# Sort test scripts for consistent execution order
IFS=$'\n' test_scripts=($(sort <<<"${test_scripts[*]}"))
unset IFS

# Run each test script
total_failures=0
for script in "${test_scripts[@]}"; do
    echo -e "=============================="
    echo -e "$script" "$@"
    echo -e "=============================="
    "$script" "$@"
    result=$?
    total_failures=$((total_failures + result))
    echo ""
done

# Display overall result
if [ $total_failures -eq 0 ]; then
    echo -e "${COLOR_BOLD}${COLOR_GREEN}=== All test suites passed ===${COLOR_RESET}"
    exit 0
else
    echo -e "${COLOR_BOLD}${COLOR_RED}=== $total_failures test suite(s) failed ===${COLOR_RESET}"
    exit 1
fi
