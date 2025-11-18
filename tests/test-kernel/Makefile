# Unit Test Kernel Makefile
# Build minimal test kernels for running unit tests in QEMU

SHELL := /bin/bash

# Supported architectures
ALL_ARCHS := amd64 arm64 riscv

# Build directory
BUILD_DIR := ../../build-test/test-kernel

# Colors
RED := \e[0;31m
GREEN := \e[0;32m
BLUE := \e[0;34m
NC := \e[0m

# Architecture configuration
ARCH_riscv_CC := $(shell which riscv64-elf-gcc 2>/dev/null || which riscv64-linux-gnu-gcc 2>/dev/null || echo riscv64-elf-gcc)
ARCH_riscv_FLAGS := -march=rv64ima_zicsr -mabi=lp64 -mcmodel=medany -mno-relax -fno-strict-aliasing
ARCH_riscv_QEMU := qemu-system-riscv64
ARCH_riscv_QEMU_FLAGS := -machine virt -bios default -m 128M
ARCH_riscv_NAME := "RISC-V"

ARCH_arm64_CC := $(shell which aarch64-elf-gcc 2>/dev/null || which aarch64-linux-gnu-gcc 2>/dev/null || echo aarch64-elf-gcc)
ARCH_arm64_FLAGS := -march=armv8-a+nolse -mabi=lp64 -mgeneral-regs-only -mno-outline-atomics -mstrict-align
ARCH_arm64_QEMU := qemu-system-aarch64
ARCH_arm64_QEMU_FLAGS := -machine virt -cpu cortex-a53 -m 128M
ARCH_arm64_NAME := "ARM64"

ARCH_amd64_CC := $(shell which x86_64-elf-gcc 2>/dev/null || which x86_64-linux-gnu-gcc 2>/dev/null || which gcc 2>/dev/null || echo x86_64-elf-gcc)
ARCH_amd64_FLAGS := -mcmodel=kernel -mno-red-zone -mno-sse -mno-sse2
ARCH_amd64_QEMU := qemu-system-x86_64
ARCH_amd64_QEMU_FLAGS := -machine pc -cpu qemu64 -m 128M -device isa-debug-exit,iobase=0xf4,iosize=0x04
ARCH_amd64_NAME := "AMD64"

.DEFAULT_GOAL := all

# Generate architecture-specific targets
define ARCH_TEMPLATE
.PHONY: _$(2)_$(1)
_$(2)_$(1):
	@$$(MAKE) --no-print-directory ARCH=$(1) _do_$(2)
endef

$(foreach arch,$(ALL_ARCHS),$(eval $(call ARCH_TEMPLATE,$(arch),build)))
$(foreach arch,$(ALL_ARCHS),$(eval $(call ARCH_TEMPLATE,$(arch),test)))

# Multi-architecture dispatch
define MULTI_ARCH_TARGET
.PHONY: $(1)
$(1):
ifeq ($$(ARCH),)
	@printf "$$(BLUE)$(2) all architectures...$$(NC)\n"
	@$$(MAKE) --no-print-directory _$(1)_amd64
	@$$(MAKE) --no-print-directory _$(1)_arm64
	@$$(MAKE) --no-print-directory _$(1)_riscv
	@printf "$$(GREEN)$(3)$$(NC)\n"
else
	@$$(MAKE) --no-print-directory _$(1)_$$(ARCH)
endif
endef

$(eval $(call MULTI_ARCH_TARGET,build,Building,All test kernels built!))
$(eval $(call MULTI_ARCH_TARGET,test,Testing,All unit tests passed!))

# Simple targets
.PHONY: all clean help

all:
ifeq ($(ARCH),)
	@$(MAKE) --no-print-directory build test
else
	@$(MAKE) --no-print-directory ARCH=$(ARCH) build test
endif

clean:
	@printf "$(BLUE)Cleaning test kernel builds...$(NC)\n"
	rm -rf $(BUILD_DIR)
	@printf "$(GREEN)Clean complete$(NC)\n"

help:
	@printf "$(BLUE)YasouOS Unit Test Kernel Build System$(NC)\n"
	@printf "\n"
	@printf "Usage: make [target] [ARCH=<arch>]\n"
	@printf "\n"
	@printf "Architectures:\n"
	@printf "  amd64      - AMD64 (x86-64)\n"
	@printf "  arm64      - ARM64\n"
	@printf "  riscv      - RISC-V\n"
	@printf "\n"
	@printf "Targets:\n"
	@printf "  all         - Build and test all architectures (default)\n"
	@printf "  build       - Build test kernel(s)\n"
	@printf "  test        - Run unit test(s) in QEMU\n"
	@printf "  clean       - Remove build files\n"
	@printf "\n"
	@printf "Examples:\n"
	@printf "  make                    # Build and test all architectures\n"
	@printf "  make ARCH=amd64 build   # Build AMD64 test kernel only\n"
	@printf "  make ARCH=arm64 test    # Run ARM64 unit tests\n"
	@printf "  make ARCH=riscv build   # Build RISC-V test kernel only\n"
	@printf "\n"

# Architecture-specific configuration
ifneq ($(ARCH),)

# Set architecture variables
CC := $(ARCH_$(ARCH)_CC)
ARCH_FLAGS := $(ARCH_$(ARCH)_FLAGS)
QEMU := $(ARCH_$(ARCH)_QEMU)
QEMU_FLAGS := $(ARCH_$(ARCH)_QEMU_FLAGS)
ARCH_NAME := $(ARCH_$(ARCH)_NAME)

# Validate architecture
ifeq ($(CC),)
    $(error Unknown architecture: $(ARCH). Run 'make help' to see supported architectures)
endif

# Common directories
COMMON_DIR := ../../common
PLATFORM_DIR := ../../kernel/platform/$(ARCH)

# Compiler flags
CFLAGS := -std=c23 -O2 -Wall -Wextra -ffreestanding -nostdlib
CFLAGS += -fno-stack-protector -fno-pic -fno-pie -fomit-frame-pointer
CFLAGS += -I$(COMMON_DIR) $(ARCH_FLAGS)
CFLAGS += -DARCH_NAME=\"$(ARCH_NAME)\"

# Linker script
LDSCRIPT := $(PLATFORM_DIR)/kernel.ld

# Find all test files and their dependencies
TEST_FILES := $(shell find ../../ -name "*.test.c" 2>/dev/null)

# Get test dependencies for each test file
define get_test_deps
$(shell dir=$$(dirname $(1)); base=$$(basename $(1) .test.c); \
	if [ -f "$$dir/$$base.c" ]; then echo "$$dir/$$base.c"; fi)
endef

# Source files
BOOT_SOURCE := $(PLATFORM_DIR)/boot_kernel.S
PLATFORM_SOURCE := $(PLATFORM_DIR)/platform.c
COMMON_SOURCE := test_kernel_common.c
KERNEL_STUBS := test_kernel_stubs.c

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Rule to build a single test kernel
define BUILD_TEST_TEMPLATE
$(BUILD_DIR)/$(notdir $(basename $(1))).$(ARCH).elf: $(1) $(call get_test_deps,$(1)) $(BOOT_SOURCE) $(PLATFORM_SOURCE) $(COMMON_SOURCE) $(KERNEL_STUBS) $(LDSCRIPT) | $(BUILD_DIR)
	@printf "$(BLUE)Building $(ARCH) test: $(notdir $(basename $(1)))$(NC)\n"
	$(CC) $(CFLAGS) -T$(LDSCRIPT) \
		$(BOOT_SOURCE) $(PLATFORM_SOURCE) $(COMMON_SOURCE) $(KERNEL_STUBS) $(1) $(call get_test_deps,$(1)) \
		-o $$@
	@printf "$(GREEN)Built: $$@ ($$(du -h $$@ | cut -f1))$(NC)\n"
endef

# Generate build rules for all test files
$(foreach test,$(TEST_FILES),$(eval $(call BUILD_TEST_TEMPLATE,$(test))))

# List of all output ELF files
TEST_ELFS := $(foreach test,$(TEST_FILES),$(BUILD_DIR)/$(notdir $(basename $(test))).$(ARCH).elf)

# Implementation targets
.PHONY: _do_build _do_test

_do_build: $(TEST_ELFS)

_do_test: $(TEST_ELFS)
	@mkdir -p $(BUILD_DIR)
	@failed=0; \
	for elf in $(TEST_ELFS); do \
		test_name=$$(basename $$elf .$(ARCH).elf); \
		test_source=$$(find ../../ -name "$${test_name}.c" 2>/dev/null | head -1 | sed 's|^\.\./\.\./||'); \
		printf "$(BLUE)Running $(ARCH)  $$test_source$(NC)  "; \
		output_file=$(BUILD_DIR)/$${test_name}.$(ARCH).log; \
		$(QEMU) $(QEMU_FLAGS) \
			-kernel $$elf \
			-nographic --no-reboot > $$output_file 2>&1 & \
		qemu_pid=$$!; \
		elapsed=0; \
		timeout=5; \
		while kill -0 $$qemu_pid 2>/dev/null; do \
			if [ $$elapsed -ge $$timeout ]; then \
				printf "$(RED)✗ timed out after $${timeout}s$(NC)\n"; \
				kill -9 $$qemu_pid 2>/dev/null; \
				wait $$qemu_pid 2>/dev/null; \
				printf "$(BLUE)--- Output ---$(NC)\n"; \
				cat $$output_file | sed 's/^/    /'; \
				printf "$(BLUE)--- End ---$(NC)\n"; \
				failed=$$((failed + 1)); \
				break; \
			fi; \
			sleep 0.1; \
			elapsed=$$((elapsed + 1)); \
		done; \
		wait $$qemu_pid 2>/dev/null; \
		if [ $$elapsed -lt $$timeout ]; then \
			if grep -q "tests starts" $$output_file && \
			   grep -q "Tests passed:" $$output_file && \
			   grep -q "tests failed: 0" $$output_file; then \
				printf "$(GREEN)✓ completed$(NC)\n"; \
			else \
				printf "$(RED)✗ failed$(NC)\n"; \
				failed=$$((failed + 1)); \
			fi; \
			printf "$(BLUE)--- Output ---$(NC)\n"; \
			cat $$output_file | sed 's/^/    /'; \
			printf "$(BLUE)--- End ---$(NC)\n"; \
		fi; \
	done; \
	if [ $$failed -gt 0 ]; then \
		printf "$(RED)$$failed test(s) failed$(NC)\n" >&2; \
		exit 1; \
	else \
		printf "$(GREEN)All tests passed!$(NC)\n"; \
	fi

endif # ARCH
