# Device Manager

Central coordinator for all devices and drivers.

## Implementation Status

**Implemented:**
- Full FDT parsing (RISC-V/ARM64)
- Bus enumeration (FDT, PCI)
- VirtIO MMIO detection with hardware probing
- Device registry (linked list + tree hierarchy)
- Driver registration with versioning
- Device-driver matching (compatible strings, vendor/device IDs)
- Probe/remove lifecycle
- Device state tracking (DISCOVERED → BOUND → ACTIVE → REMOVED)
- MMIO mapping (identity mapping)

**Not Implemented:**
- IRQ handling
- Hotplug support
- DMA allocation
- Power management
- Virtual memory MMIO mapping

## Device Interaction Methods

| Method | Architecture | Implementation |
|--------|-------------|----------------|
| MMIO | All | Identity mapping via `device_map_mmio()` |
| PCIe ECAM | AMD64 | Config space at 0xB0000000 |
| Platform Devices (FDT) | RISC-V, ARM64, AMD64 | FDT parser |

**Limitations:**
- MMIO uses identity mapping (physical == virtual)
- PCI scans first 2 buses, function 0 only
- No hotplug (static discovery at boot)

## Bus Enumerators

### FDT Parser (RISC-V/ARM64)
**Location:** [kernel/platform/fdt_parser.c](../kernel/platform/fdt_parser.c)

- Parses FDT blob from bootloader
- Extracts name, compatible strings, reg addresses/sizes
- VirtIO detection: reads magic (0x74726976) and device type from MMIO registers
- Reports only devices with attached hardware

### PCIe ECAM (AMD64)
**Location:** [kernel/devices/devices_amd64.c](../kernel/devices/devices_amd64.c)

- Scans PCIe config space via ECAM at 0xB0000000
- Reads vendor/device IDs and BARs
- Scans bus 0 only (sufficient for QEMU)
- Falls back to FDT if available

## Boot Sequence

1. Device Manager init (registry, driver table)
2. Bus enumeration (PCI, FDT)
3. Driver binding (`drivers_probe_all()`)
4. Hotplug monitoring (future)

## Device Lifecycle

**State Transitions:**
1. `devices_scan()` → **DISCOVERED**
2. `drivers_probe_all()` → **BOUND**
3. `driver->ops->probe()` → **ACTIVE** (future)
4. `device_remove()` → **REMOVED** (future)

See [kernel/devices/devices.h](../kernel/devices/devices.h) for `device_state_t`.

## Device Tree Structure

Each device maintains:
- **Tree pointers:** `parent`, `first_child`, `next_sibling`
- **Flat list pointer:** `next` (for simple iteration)

**Current Limitation:** VirtIO MMIO creates flat list only. Tree hierarchy requires full FDT parsing.
