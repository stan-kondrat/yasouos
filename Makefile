# YasouOS - Simple multi-architecture OS build system

# Supported architectures
SUPPORTED_ARCHS := riscv arm64 amd64

# Directories
INCLUDE_DIR := include
LIB_DIR := lib
ARCH_DIR = arch/$(ARCH)
BUILD_DIR = build/$(ARCH)

# Colors
RED := \033[0;31m
GREEN := \033[0;32m
YELLOW := \033[0;33m
BLUE := \033[0;34m
NC := \033[0m

# Architecture configuration
ARCH_riscv_CC := $(shell which riscv64-elf-gcc 2>/dev/null || which riscv64-linux-gnu-gcc 2>/dev/null || echo riscv64-elf-gcc)
ARCH_riscv_FLAGS := -march=rv64ima -mabi=lp64 -mcmodel=medany -mno-relax -fno-strict-aliasing
ARCH_riscv_QEMU := qemu-system-riscv64
ARCH_riscv_QEMU_FLAGS := -machine virt -bios default
ARCH_riscv_QEMU_FLAGS_IMG := -machine virt -bios default
ARCH_riscv_NAME := "RISC-V 64-bit"

ARCH_arm64_CC := $(shell which aarch64-elf-gcc 2>/dev/null || which aarch64-linux-gnu-gcc 2>/dev/null || echo aarch64-elf-gcc)
ARCH_arm64_FLAGS := -march=armv8-a -mabi=lp64
ARCH_arm64_QEMU := qemu-system-aarch64
ARCH_arm64_QEMU_FLAGS := -machine virt -cpu cortex-a53 -semihosting-config enable=on,target=native
ARCH_arm64_QEMU_FLAGS_IMG := -machine virt -cpu cortex-a53 -semihosting-config enable=on,target=native
ARCH_arm64_NAME := "ARM64/AArch64"

ARCH_amd64_CC := $(shell which x86_64-elf-gcc 2>/dev/null || which x86_64-linux-gnu-gcc 2>/dev/null || which gcc 2>/dev/null || echo x86_64-elf-gcc)
ARCH_amd64_FLAGS := -m32
ARCH_amd64_QEMU := qemu-system-x86_64
ARCH_amd64_QEMU_FLAGS := -machine q35 -cpu qemu64 -m 128M -device isa-debug-exit,iobase=0xf4,iosize=0x04
ARCH_amd64_QEMU_FLAGS_IMG := -machine pc -cpu qemu64 -m 128M -device isa-debug-exit,iobase=0xf4,iosize=0x04
ARCH_amd64_NAME := "AMD64/x86-64"

.DEFAULT_GOAL := all

# Generate architecture-specific targets
define ARCH_TEMPLATE
.PHONY: _$(2)_$(1)
_$(2)_$(1):
	@$$(MAKE) --no-print-directory ARCH=$(1) _do_$(2)
endef

$(foreach arch,$(SUPPORTED_ARCHS),$(eval $(call ARCH_TEMPLATE,$(arch),build)))
$(foreach arch,$(SUPPORTED_ARCHS),$(eval $(call ARCH_TEMPLATE,$(arch),test)))
$(foreach arch,$(SUPPORTED_ARCHS),$(eval $(call ARCH_TEMPLATE,$(arch),test-img)))
$(foreach arch,$(SUPPORTED_ARCHS),$(eval $(call ARCH_TEMPLATE,$(arch),run-img)))

# Multi-architecture dispatch
define MULTI_ARCH_TARGET
.PHONY: $(1)
$(1):
ifeq ($$(ARCH),)
	@echo "$$(BLUE)$(2) all architectures...$$(NC)"
	@$$(MAKE) --no-print-directory _$(1)_riscv
	@$$(MAKE) --no-print-directory _$(1)_arm64
	@$$(MAKE) --no-print-directory _$(1)_amd64
	@echo "$$(GREEN)$(3)$$(NC)"
else
	@$$(MAKE) --no-print-directory _$(1)_$$(ARCH)
endif
endef

$(eval $(call MULTI_ARCH_TARGET,build,Building,All architectures built!))
$(eval $(call MULTI_ARCH_TARGET,test,Testing,All tests passed!))
$(eval $(call MULTI_ARCH_TARGET,test-img,Testing disk images for,All disk image tests passed!))

# Simple targets
.PHONY: all clean help
all:
ifeq ($(ARCH),)
	@$(MAKE) --no-print-directory build test
else
	@$(MAKE) --no-print-directory ARCH=$(ARCH) build test
endif

clean:
	@echo "$(BLUE)Cleaning build files...$(NC)"
	rm -rf build
	@echo "$(GREEN)Clean complete$(NC)"

help:
	@echo "$(BLUE)YasouOS Build System$(NC)"
	@echo ""
	@echo "Usage: make [target] [ARCH=riscv|arm64|amd64]"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build and test all architectures (default)"
	@echo "  build      - Build kernel(s) and disk images"
	@echo "  run        - Build and run in QEMU (requires ARCH)"
	@echo "  run-img    - Run bootable disk image interactively (requires ARCH)"
	@echo "  test       - Test kernel(s)"
	@echo "  test-img   - Test bootable disk images"
	@echo "  clean      - Remove build files"
	@echo "  check-deps - Check build dependencies"
	@echo ""

# Single-architecture targets
define SINGLE_ARCH_TARGET
.PHONY: $(1)
$(1):
ifeq ($$(ARCH),)
	@echo "$$(RED)Error: ARCH must be specified for $(1) target$$(NC)"
	@echo "Usage: make ARCH=riscv $(1)"
	@exit 1
else
	@$$(MAKE) --no-print-directory ARCH=$$(ARCH) _do_$(1)
endif
endef

$(eval $(call SINGLE_ARCH_TARGET,run))
$(eval $(call SINGLE_ARCH_TARGET,run-img))

# Dependency checking
.PHONY: check-deps
check-deps:
ifeq ($(ARCH),)
	@echo "$(BLUE)Checking build dependencies for all architectures...$(NC)"
	@$(foreach arch,$(SUPPORTED_ARCHS),$(MAKE) --no-print-directory ARCH=$(arch) _do_check_deps;)
else
	@$(MAKE) --no-print-directory ARCH=$(ARCH) _do_check_deps
endif









# Architecture-specific configuration (only when ARCH is specified)
ifneq ($(ARCH),)

# Set architecture variables
CC := $(ARCH_$(ARCH)_CC)
ARCH_FLAGS := $(ARCH_$(ARCH)_FLAGS)
QEMU := $(ARCH_$(ARCH)_QEMU)
QEMU_FLAGS := $(ARCH_$(ARCH)_QEMU_FLAGS)
QEMU_FLAGS_IMG := $(ARCH_$(ARCH)_QEMU_FLAGS_IMG)
ARCH_NAME := $(ARCH_$(ARCH)_NAME)

# Validate architecture
ifeq ($(CC),)
    $(error Unknown architecture: $(ARCH). Use ARCH=riscv, ARCH=arm64, or ARCH=amd64)
endif

# Set toolchain
COMPILER_PREFIX := $(CC:%gcc=%)
ifeq ($(COMPILER_PREFIX),$(CC))
    AS := as
    LD := ld
    OBJCOPY := objcopy
    OBJDUMP := objdump
else
    AS := $(COMPILER_PREFIX)as
    LD := $(COMPILER_PREFIX)ld
    OBJCOPY := $(COMPILER_PREFIX)objcopy
    OBJDUMP := $(COMPILER_PREFIX)objdump
endif

# AMD64 fallback to native compiler
ifeq ($(shell which $(CC) 2>/dev/null),)
    ifeq ($(ARCH),amd64)
        ifeq ($(shell uname -m),x86_64)
            CC := gcc
            AS := as
            LD := ld
            OBJCOPY := objcopy
            OBJDUMP := objdump
        endif
    endif
endif

# Build files
KERNEL_ELF := $(BUILD_DIR)/kernel.elf
KERNEL_BIN := $(BUILD_DIR)/kernel.bin
KERNEL_DISK_ELF := $(BUILD_DIR)/kernel_disk.elf
KERNEL_DISK_BIN := $(BUILD_DIR)/kernel_disk.bin
DISK_IMG := $(BUILD_DIR)/disk.img
MAP := $(BUILD_DIR)/ld.map
DISK_MAP := $(BUILD_DIR)/ld_disk.map

# Compiler flags
CFLAGS := -std=c23 -O3 -g3 -Wall -Wextra -ffreestanding -nostdlib -fno-builtin
CFLAGS += -fno-stack-protector -fno-pic -fno-pie -I$(INCLUDE_DIR) $(ARCH_FLAGS)
CFLAGS += -DARCH_NAME=\"$(ARCH_NAME)\"

# Linker flags
LDFLAGS := -nostdlib -static -no-pie -T$(ARCH_DIR)/kernel.ld -Wl,-Map=$(MAP)

# Source files
C_SOURCES := $(LIB_DIR)/kernel.c $(LIB_DIR)/common.c $(ARCH_DIR)/platform.c
ASM_SOURCES := $(ARCH_DIR)/boot_kernel.S
ifeq ($(ARCH),amd64)
ASM_DISK_SOURCES := $(ARCH_DIR)/boot_disk.S
endif
C_OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(C_SOURCES)))
ASM_OBJECTS := $(patsubst %.S,$(BUILD_DIR)/%.o,$(notdir $(ASM_SOURCES)))
OBJECTS := $(ASM_OBJECTS) $(C_OBJECTS)

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Build kernel
$(KERNEL_ELF): $(OBJECTS) | $(BUILD_DIR)
	@echo "$(BLUE)Linking kernel for $(ARCH)...$(NC)"
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS)
	@echo "$(GREEN)Kernel built: $@ ($$(du -h $@ | cut -f1))$(NC)"

# Build disk kernel (AMD64 only)
ifeq ($(ARCH),amd64)
ASM_DISK_OBJECTS := $(patsubst %.S,$(BUILD_DIR)/%_disk.o,$(notdir $(ASM_DISK_SOURCES)))
C_DISK_OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%_disk.o,$(notdir $(C_SOURCES)))
DISK_OBJECTS := $(ASM_DISK_OBJECTS) $(C_DISK_OBJECTS)

$(BUILD_DIR)/%_disk.o: $(LIB_DIR)/%.c | $(BUILD_DIR)
	@echo "$(BLUE)Compiling $< for disk boot...$(NC)"
	$(CC) $(CFLAGS) -DDISK_BOOT -c $< -o $@

$(BUILD_DIR)/%_disk.o: $(ARCH_DIR)/%.c | $(BUILD_DIR)
	@echo "$(BLUE)Compiling $< for disk boot...$(NC)"
	$(CC) $(CFLAGS) -DDISK_BOOT -c $< -o $@

$(BUILD_DIR)/boot_kernel_disk.o: $(ARCH_DIR)/boot_kernel.S | $(BUILD_DIR)
	@echo "$(BLUE)Assembling $< for disk boot...$(NC)"
	$(CC) $(CFLAGS) -DDISK_BOOT -c $< -o $@

$(BUILD_DIR)/boot_disk_disk.o: $(ARCH_DIR)/boot_disk.S | $(BUILD_DIR)
	@echo "$(BLUE)Assembling $< for disk boot...$(NC)"
	$(CC) $(CFLAGS) -DDISK_BOOT -c $< -o $@

$(KERNEL_DISK_ELF): $(DISK_OBJECTS) | $(BUILD_DIR)
	@echo "$(BLUE)Linking disk kernel for $(ARCH)...$(NC)"
	$(CC) $(CFLAGS) -DDISK_BOOT -nostdlib -static -no-pie -T$(ARCH_DIR)/kernel_disk.ld -Wl,-Map=$(DISK_MAP) -o $@ $(DISK_OBJECTS)
	@echo "$(GREEN)Disk kernel built: $@ ($$(du -h $@ | cut -f1))$(NC)"

$(KERNEL_DISK_BIN): $(KERNEL_DISK_ELF)
	@echo "$(BLUE)Extracting disk kernel binary...$(NC)"
	$(OBJCOPY) -O binary $< $@
	@echo "$(GREEN)Disk kernel binary: $@ ($$(du -h $@ | cut -f1))$(NC)"
endif

# Pattern rules for compilation
$(BUILD_DIR)/%.o: $(LIB_DIR)/%.c | $(BUILD_DIR)
	@echo "$(BLUE)Compiling $<...$(NC)"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(ARCH_DIR)/%.c | $(BUILD_DIR)
	@echo "$(BLUE)Compiling $<...$(NC)"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(ARCH_DIR)/%.S | $(BUILD_DIR)
	@echo "$(BLUE)Assembling $<...$(NC)"
	$(CC) $(CFLAGS) -c $< -o $@

# Extract kernel binary
$(KERNEL_BIN): $(KERNEL_ELF)
	@echo "$(BLUE)Extracting kernel binary...$(NC)"
	$(OBJCOPY) -O binary $< $@
	@echo "$(GREEN)Kernel binary: $@ ($$(du -h $@ | cut -f1))$(NC)"

# Create disk image
ifeq ($(ARCH),amd64)
BOOTLOADER := $(BUILD_DIR)/bootloader.bin

$(BOOTLOADER): $(ARCH_DIR)/boot_disk.S | $(BUILD_DIR)
	@echo "$(BLUE)Building bootloader...$(NC)"
	$(AS) --32 -o $(BUILD_DIR)/bootloader_temp.o -defsym BOOTLOADER_ONLY=1 $<
	$(LD) -m elf_i386 --oformat binary -Ttext 0x7C00 -o $@ $(BUILD_DIR)/bootloader_temp.o
	@dd if=$@ of=$@.tmp bs=510 count=1 2>/dev/null
	@printf '\x55\xAA' >> $@.tmp
	@mv $@.tmp $@
	@echo "$(GREEN)Bootloader built: $@ (512 bytes)$(NC)"

$(DISK_IMG): $(BOOTLOADER) $(KERNEL_DISK_BIN)
	@echo "$(BLUE)Creating AMD64 disk image...$(NC)"
	dd if=/dev/zero of=$@ bs=1M count=10 2>/dev/null
	dd if=$(BOOTLOADER) of=$@ bs=512 count=1 conv=notrunc 2>/dev/null
	dd if=$(KERNEL_DISK_BIN) of=$@ bs=512 seek=1 conv=notrunc 2>/dev/null
	@echo "$(GREEN)Disk image created: $@$(NC)"
else
$(DISK_IMG): $(KERNEL_BIN)
	@echo "$(BLUE)Creating $(ARCH) disk image...$(NC)"
	dd if=/dev/zero of=$@ bs=1M count=10 2>/dev/null
	dd if=$(KERNEL_BIN) of=$@ conv=notrunc 2>/dev/null
	@echo "$(GREEN)Disk image created: $@$(NC)"
endif

# Implementation targets
.PHONY: _do_build _do_test _do_test-img _do_run _do_run-img _do_check_deps
_do_build: $(KERNEL_ELF) $(DISK_IMG)

_do_run: $(KERNEL_ELF)
	@echo "$(BLUE)Starting YasouOS in QEMU ($(ARCH))...$(NC)"
	@echo "$(YELLOW)Press Ctrl-A X to exit$(NC)"
	$(QEMU) $(QEMU_FLAGS) -kernel $(KERNEL_ELF) -nographic --no-reboot || true

_do_run-img: $(DISK_IMG)
	@echo "$(BLUE)Running $(ARCH) disk image in QEMU...$(NC)"
	@echo "$(YELLOW)Press Ctrl-A X to exit$(NC)"
ifeq ($(ARCH),amd64)
	$(QEMU) $(QEMU_FLAGS_IMG) -drive file=$(DISK_IMG),format=raw -nographic --no-reboot || true
else
	@echo "$(YELLOW)Not implemented yet.$(NC)"
endif

_do_test: $(KERNEL_ELF)
	@echo "$(BLUE)Testing YasouOS ($(ARCH))...$(NC)"
	@{ $(QEMU) $(QEMU_FLAGS) -kernel $(KERNEL_ELF) -nographic --no-reboot 2>&1 || true; } | tee $(BUILD_DIR)/test-$(ARCH).log
	@if grep -q "Hello World from YasouOS!" $(BUILD_DIR)/test-$(ARCH).log; then \
		echo "$(GREEN)✓ Test passed for $(ARCH)$(NC)"; \
	else \
		echo "$(RED)✗ Test failed for $(ARCH)$(NC)"; \
		exit 1; \
	fi

_do_test-img: $(DISK_IMG)
	@echo "$(BLUE)Testing $(ARCH) disk image in QEMU...$(NC)"
ifeq ($(ARCH),amd64)
	@{ $(QEMU) $(QEMU_FLAGS_IMG) -drive file=$(DISK_IMG),format=raw -nographic --no-reboot || true; } 2>&1 | tee $(BUILD_DIR)/test-img-$(ARCH).log
else
	@echo "$(YELLOW)Not implemented yet.$(NC)"
endif
	@if grep -q "Hello World from YasouOS!" $(BUILD_DIR)/test-img-$(ARCH).log; then \
		echo "$(GREEN)✓ Disk image test passed for $(ARCH)$(NC)"; \
	else \
		echo "$(RED)✗ Disk image test failed for $(ARCH)$(NC)"; \
		exit 1; \
	fi

_do_check_deps:
	@echo "$(BLUE)Checking $(ARCH) dependencies...$(NC)"
	@echo "Architecture: $(ARCH_NAME)"
	@echo "Compiler ($(CC)): "
	@if which $(CC) >/dev/null 2>&1; then \
		echo "$(GREEN)✓ found$(NC)"; \
	else \
		echo "$(RED)✗ not found$(NC)"; \
	fi
	@echo "QEMU ($(QEMU)): "
	@if which $(QEMU) >/dev/null 2>&1; then \
		echo "$(GREEN)✓ found$(NC)"; \
	else \
		echo "$(RED)✗ not found$(NC)"; \
	fi
	@echo ""

endif
