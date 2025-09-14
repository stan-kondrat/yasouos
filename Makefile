# YasouOS Makefile - Multi-architecture OS build system

# Architecture (no default - when empty, build all architectures)

# Common directories
INCLUDE_DIR := include
LIB_DIR := lib
ARCH_DIR = arch/$(ARCH)
BUILD_DIR = build/$(ARCH)

# Colors for output
RED := \033[0;31m
GREEN := \033[0;32m
YELLOW := \033[0;33m
BLUE := \033[0;34m
NC := \033[0m

# Default target - build and test all architectures if no ARCH specified
.PHONY: all
all:
ifeq ($(ARCH),)
	@$(MAKE) --no-print-directory build test
else
	@$(MAKE) --no-print-directory ARCH=$(ARCH) build test
endif

# Help
.PHONY: help
help:
	@echo "$(BLUE)YasouOS Build System$(NC)"
	@echo ""
	@echo "Usage: make [target] [ARCH=riscv|arm64|amd64]"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build and test all architectures (default)"
	@echo "  build      - Build kernel(s) - all arches if ARCH not specified"
	@echo "  test       - Test kernel(s) - all arches if ARCH not specified"
	@echo "  run        - Build and run in QEMU (requires ARCH)"
	@echo "  clean      - Remove build files"
	@echo "  check-deps - Check build dependencies"
	@echo "  help       - Show this help"
	@echo ""
	@echo "Architecture-specific usage:"
	@echo "  make ARCH=riscv       # Build and test RISC-V only"
	@echo "  make ARCH=arm64 build # Build ARM64 only"
	@echo "  make ARCH=amd64 run   # Build and run AMD64"
	@echo ""
	@echo "Cross-compiler requirements:"
	@echo "  RISC-V: riscv64-unknown-elf-gcc"
	@echo "  ARM64:  aarch64-elf-gcc"
	@echo "  AMD64:  x86_64-elf-gcc (or native gcc on x86_64)"

# Clean
.PHONY: clean
clean:
	@echo "$(BLUE)Cleaning build files...$(NC)"
	rm -rf build
	@echo "$(GREEN)Clean complete$(NC)"


# Check dependencies - optimized version leveraging existing architecture detection
.PHONY: check-deps
check-deps:
ifeq ($(ARCH),)
	@echo "$(BLUE)Checking build dependencies for all architectures...$(NC)"
	@echo ""
	@$(MAKE) --no-print-directory ARCH=riscv check-deps
	@$(MAKE) --no-print-directory ARCH=arm64 check-deps
	@$(MAKE) --no-print-directory ARCH=amd64 check-deps
else
	@echo "$(BLUE)Checking $(ARCH) dependencies...$(NC)"
	@echo "Architecture: $(ARCH_NAME)"
	@echo -n "Compiler ($(CC)): "
	@if which $(CC) >/dev/null 2>&1; then \
		echo "$(GREEN)✓ found$(NC)"; \
	else \
		echo "$(RED)✗ not found$(NC)"; \
	fi
	@echo -n "QEMU ($(QEMU)): "
	@if which $(QEMU) >/dev/null 2>&1; then \
		echo "$(GREEN)✓ found$(NC)"; \
	else \
		echo "$(RED)✗ not found$(NC)"; \
	fi
	@echo ""
endif

# Build target - builds all architectures or specified ARCH
.PHONY: build
build:
ifeq ($(ARCH),)
	@echo "$(BLUE)Building all architectures...$(NC)"
	@$(MAKE) --no-print-directory ARCH=riscv _build_single
	@$(MAKE) --no-print-directory ARCH=arm64 _build_single
	@$(MAKE) --no-print-directory ARCH=amd64 _build_single
	@echo "$(GREEN)All architectures built!$(NC)"
else
	@$(MAKE) --no-print-directory ARCH=$(ARCH) _build_single
endif

# Test target - tests all architectures or specified ARCH
.PHONY: test
test:
ifeq ($(ARCH),)
	@echo "$(BLUE)Testing all architectures...$(NC)"
	@$(MAKE) --no-print-directory ARCH=riscv _test_single
	@$(MAKE) --no-print-directory ARCH=arm64 _test_single
	@$(MAKE) --no-print-directory ARCH=amd64 _test_single
	@echo "$(GREEN)All tests passed!$(NC)"
else
	@$(MAKE) --no-print-directory ARCH=$(ARCH) _test_single
endif

# Architecture-specific configuration (only when ARCH is specified)
ifneq ($(ARCH),)

# Architecture-specific files
KERNEL := $(BUILD_DIR)/yasouos-$(ARCH).elf
MAP := $(BUILD_DIR)/yasouos-$(ARCH).map

# Compiler and QEMU configuration by architecture
ifeq ($(ARCH),riscv)
    # Detect available RISC-V compiler
    ifneq ($(shell which riscv64-elf-gcc 2>/dev/null),)
        CC := riscv64-elf-gcc
    else ifneq ($(shell which riscv64-linux-gnu-gcc 2>/dev/null),)
        CC := riscv64-linux-gnu-gcc
    else ifneq ($(shell which riscv64-unknown-elf-gcc 2>/dev/null),)
        CC := riscv64-unknown-elf-gcc
    else
        CC := riscv64-elf-gcc
    endif
    ARCH_FLAGS := -march=rv64ima -mabi=lp64 -mcmodel=medany -mno-relax
    CFLAGS += -fno-strict-aliasing
    QEMU := qemu-system-riscv64
    QEMU_MACHINE := virt
    QEMU_FLAGS := -bios default
    ARCH_NAME := "RISC-V 64-bit"
else ifeq ($(ARCH),arm64)
    # Detect available ARM64 compiler
    ifneq ($(shell which aarch64-elf-gcc 2>/dev/null),)
        CC := aarch64-elf-gcc
    else ifneq ($(shell which aarch64-linux-gnu-gcc 2>/dev/null),)
        CC := aarch64-linux-gnu-gcc
    else
        CC := aarch64-elf-gcc
    endif
    ARCH_FLAGS := -march=armv8-a -mabi=lp64
    QEMU := qemu-system-aarch64
    QEMU_MACHINE := virt
    QEMU_FLAGS := -cpu cortex-a53 -semihosting-config enable=on,target=native
    ARCH_NAME := "ARM64/AArch64"
else ifeq ($(ARCH),amd64)
    # Detect available x86-64 compiler
    ifneq ($(shell which x86_64-elf-gcc 2>/dev/null),)
        CC := x86_64-elf-gcc
    else ifneq ($(shell which x86_64-linux-gnu-gcc 2>/dev/null),)
        CC := x86_64-linux-gnu-gcc
    else ifneq ($(shell which gcc 2>/dev/null),)
        CC := gcc
    else
        CC := x86_64-elf-gcc
    endif
    ARCH_FLAGS := -m32
    QEMU := qemu-system-x86_64
    QEMU_MACHINE := q35
    QEMU_FLAGS := -cpu qemu64 -m 128M -device isa-debug-exit,iobase=0xf4,iosize=0x04
    ARCH_NAME := "AMD64/x86-64"
else
    $(error Unknown architecture: $(ARCH). Use ARCH=riscv, ARCH=arm64, or ARCH=amd64)
endif

# Set other tools based on compiler
COMPILER_PREFIX := $(subst gcc,,$(CC))
ifeq ($(COMPILER_PREFIX),$(CC))
    # Native compiler (gcc-14, etc)
    AS := as
    LD := ld
    OBJCOPY := objcopy
    OBJDUMP := objdump
else
    # Cross compiler
    AS := $(COMPILER_PREFIX)as
    LD := $(COMPILER_PREFIX)ld
    OBJCOPY := $(COMPILER_PREFIX)objcopy
    OBJDUMP := $(COMPILER_PREFIX)objdump
endif

# Check if compiler is available, fallback for AMD64 only
ifeq ($(shell which $(CC) 2>/dev/null),)
    ifeq ($(ARCH),amd64)
        ifeq ($(shell uname -m),x86_64)
            CC := gcc
            AS := as
            LD := ld
            OBJCOPY := objcopy
            OBJDUMP := objdump
        else
            $(error AMD64 requires cross compiler or native x86_64 system)
        endif
    else ifeq ($(ARCH),riscv)
        $(error RISC-V requires riscv64-elf-gcc, riscv64-linux-gnu-gcc, or riscv64-unknown-elf-gcc)
    else ifeq ($(ARCH),arm64)
        $(error ARM64 requires aarch64-elf-gcc or aarch64-linux-gnu-gcc)
    endif
endif

# Compiler flags - using C23 standard
CFLAGS := -std=c23 -O3 -g3 -Wall -Wextra
CFLAGS += -ffreestanding -nostdlib -fno-builtin
CFLAGS += -fno-stack-protector -fno-pic -fno-pie
CFLAGS += -I$(INCLUDE_DIR)
CFLAGS += $(ARCH_FLAGS)
CFLAGS += -DARCH_NAME=\"$(ARCH_NAME)\"

# Linker flags
LDFLAGS := -nostdlib -static -no-pie -T$(ARCH_DIR)/kernel.ld -Wl,-Map=$(MAP)

# Source files
C_SOURCES := $(LIB_DIR)/kernel.c $(LIB_DIR)/common.c $(ARCH_DIR)/platform.c
ASM_SOURCES := $(ARCH_DIR)/boot.S

# Object files
C_OBJECTS := $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(C_SOURCES)))
ASM_OBJECTS := $(patsubst %.S,$(BUILD_DIR)/%.o,$(notdir $(ASM_SOURCES)))
OBJECTS := $(ASM_OBJECTS) $(C_OBJECTS)

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Build kernel
$(KERNEL): $(BUILD_DIR) $(OBJECTS)
	@echo "$(BLUE)Linking kernel for $(ARCH)...$(NC)"
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJECTS)
	@echo "$(GREEN)Kernel built: $@$(NC)"
	@echo "Size: $$(du -h $@ | cut -f1)"

# Compile C sources
$(BUILD_DIR)/%.o: $(LIB_DIR)/%.c
	@echo "$(BLUE)Compiling $<...$(NC)"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(ARCH_DIR)/%.c
	@echo "$(BLUE)Compiling $<...$(NC)"
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble ASM sources
$(BUILD_DIR)/%.o: $(ARCH_DIR)/%.S
	@echo "$(BLUE)Assembling $<...$(NC)"
	$(CC) $(CFLAGS) -c $< -o $@

# Single architecture build (internal target)
.PHONY: _build_single
_build_single: $(KERNEL)


# Single architecture test (internal target)
.PHONY: _test_single
_test_single: $(KERNEL)
	@echo "$(BLUE)Testing YasouOS ($(ARCH))...$(NC)"
	@rm -f $(BUILD_DIR)/test-$(ARCH).log
	@{ \
		$(QEMU) -machine $(QEMU_MACHINE) $(QEMU_FLAGS) \
		-kernel $(KERNEL) \
		-nographic \
		--no-reboot 2>&1 || true; \
	} | tee $(BUILD_DIR)/test-$(ARCH).log
	@if [ ! -s $(BUILD_DIR)/test-$(ARCH).log ]; then \
		echo "$(RED)✗ Test failed for $(ARCH) - no output generated$(NC)"; \
		echo "$(YELLOW)QEMU may have failed to start or kernel failed to boot$(NC)"; \
		exit 1; \
	elif grep -q "Hello World" $(BUILD_DIR)/test-$(ARCH).log; then \
		echo "$(GREEN)✓ Test passed for $(ARCH)$(NC)"; \
	else \
		echo "$(RED)✗ Test failed for $(ARCH)$(NC)"; \
		echo "$(YELLOW)Expected 'Hello World' in output but found:$(NC)"; \
		cat $(BUILD_DIR)/test-$(ARCH).log; \
		exit 1; \
	fi

# Run in QEMU (requires ARCH)
.PHONY: run
run:
ifeq ($(ARCH),)
	@echo "$(RED)Error: ARCH must be specified for run target$(NC)"
	@echo "Usage: make ARCH=riscv run"
	@exit 1
else
	@$(MAKE) --no-print-directory ARCH=$(ARCH) _run_single
endif

.PHONY: _run_single
_run_single: $(KERNEL)
	@echo "$(BLUE)Starting YasouOS in QEMU ($(ARCH))...$(NC)"
	@echo "$(YELLOW)Press Ctrl-A X to exit$(NC)"
	$(QEMU) -machine $(QEMU_MACHINE) $(QEMU_FLAGS) \
		-kernel $(KERNEL) \
		-nographic \
		--no-reboot

endif

.DEFAULT_GOAL := all