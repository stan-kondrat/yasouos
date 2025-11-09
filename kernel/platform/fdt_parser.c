#include "fdt_parser.h"
#include "../../common/common.h"

// ============================================================================
// FDT Header and Token Definitions
// ============================================================================

#define FDT_MAGIC 0xd00dfeed

// FDT tokens
#define FDT_BEGIN_NODE  0x00000001
#define FDT_END_NODE    0x00000002
#define FDT_PROP        0x00000003
#define FDT_NOP         0x00000004
#define FDT_END         0x00000009

typedef struct {
    uint32_t magic;
    uint32_t totalsize;
    uint32_t off_dt_struct;
    uint32_t off_dt_strings;
    uint32_t off_mem_rsvmap;
    uint32_t version;
    uint32_t last_comp_version;
    uint32_t boot_cpuid_phys;
    uint32_t size_dt_strings;
    uint32_t size_dt_struct;
} __attribute__((packed)) fdt_header_t;

typedef struct {
    uint32_t len;
    uint32_t nameoff;
} __attribute__((packed)) fdt_prop_t;

// ============================================================================
// Helper Functions
// ============================================================================

// Big-endian to CPU byte order conversion (manual implementation)
static inline uint32_t be32_to_cpu(uint32_t val) {
    return ((val & 0xFF000000) >> 24) |
           ((val & 0x00FF0000) >> 8) |
           ((val & 0x0000FF00) << 8) |
           ((val & 0x000000FF) << 24);
}

// Align pointer to 4-byte boundary
static inline uint32_t align_up(uint32_t val) {
    return (val + 3) & ~3;
}

// Read unaligned 32-bit big-endian value safely
// Use volatile to prevent compiler from optimizing into unaligned load
static inline uint32_t read_be32_unaligned(const void *ptr) {
    const volatile uint8_t *bytes = (const volatile uint8_t *)ptr;
    return ((uint32_t)bytes[0] << 24) |
           ((uint32_t)bytes[1] << 16) |
           ((uint32_t)bytes[2] << 8) |
           ((uint32_t)bytes[3]);
}

// Compare strings (returns 0 if equal)
static int str_equal(const char *s1, const char *s2) {
    return strcmp(s1, s2) == 0;
}

// ============================================================================
// FDT Parser Implementation
// ============================================================================

const char* fdt_get_bootargs(uintptr_t fdt_addr) {
    if (fdt_addr == 0) {
        return NULL;
    }

    // Parse FDT header
    fdt_header_t *header = (fdt_header_t *)fdt_addr;

    // Verify magic number
    uint32_t magic = be32_to_cpu(header->magic);
    if (magic != FDT_MAGIC) {
        return NULL;
    }

    // Get pointers to FDT sections
    uint32_t struct_offset = be32_to_cpu(header->off_dt_struct);
    uint32_t strings_offset = be32_to_cpu(header->off_dt_strings);
    uint32_t totalsize = be32_to_cpu(header->totalsize);

    uint8_t *fdt_base = (uint8_t *)fdt_addr;
    uint32_t *struct_ptr = (uint32_t *)(fdt_base + struct_offset);
    char *strings_base = (char *)(fdt_base + strings_offset);

    // State tracking
    int depth = 0;
    bool in_chosen = false;
    const char *bootargs = NULL;

    // Walk the structure block
    while (1) {
        // Safety check before reading token
        if ((uintptr_t)struct_ptr >= fdt_addr + totalsize) {
            return NULL;
        }

        uint32_t token = be32_to_cpu(*struct_ptr);
        struct_ptr++;

        switch (token) {
            case FDT_BEGIN_NODE: {
                // Safety check before reading node name
                if ((uintptr_t)struct_ptr >= fdt_addr + totalsize) {
                    return NULL;
                }

                // Read node name (null-terminated string)
                char *node_name = (char *)struct_ptr;

                // Calculate safe length for node name
                size_t max_name_len = (fdt_addr + totalsize) - (uintptr_t)struct_ptr;
                size_t name_len = 0;
                while (name_len < max_name_len && node_name[name_len] != '\0') {
                    name_len++;
                }

                // If we didn't find null terminator, FDT is corrupted
                if (name_len >= max_name_len) {
                    return NULL;
                }

                // Check if this is the /chosen node (child of root, so depth == 1 before incrementing)
                if (depth == 1 && str_equal(node_name, "chosen")) {
                    in_chosen = true;
                }

                // Advance past node name (aligned to 4 bytes)
                struct_ptr = (uint32_t *)((char *)struct_ptr + align_up(name_len + 1));
                depth++;
                break;
            }

            case FDT_END_NODE:
                depth--;
                if (depth == 1 && in_chosen) {
                    in_chosen = false;
                }
                break;

            case FDT_PROP: {
                // Safety check before reading property header
                if ((uintptr_t)struct_ptr + sizeof(fdt_prop_t) > fdt_addr + totalsize) {
                    return NULL;
                }

                // Read property structure
                fdt_prop_t *prop = (fdt_prop_t *)struct_ptr;
                uint32_t prop_len = be32_to_cpu(prop->len);
                uint32_t prop_nameoff = be32_to_cpu(prop->nameoff);
                struct_ptr += 2;  // Skip len and nameoff

                // Safety check for property data
                if ((uintptr_t)struct_ptr + prop_len > fdt_addr + totalsize) {
                    return NULL;
                }

                // Get property name from strings block
                char *prop_name = strings_base + prop_nameoff;

                // Get property data
                char *prop_data = (char *)struct_ptr;

                // Check if this is bootargs property in /chosen node
                if (in_chosen && str_equal(prop_name, "bootargs")) {
                    // bootargs is a null-terminated string
                    if (prop_len > 0) {
                        bootargs = prop_data;
                    }
                }

                // Advance past property data (aligned to 4 bytes)
                struct_ptr = (uint32_t *)((char *)struct_ptr + align_up(prop_len));
                break;
            }

            case FDT_NOP:
                // Skip NOP tokens
                break;

            case FDT_END:
                // Reached end of structure block
                return bootargs;

            default:
                // Unknown token - FDT might be corrupted
                return NULL;
        }
    }

    return bootargs;
}

int fdt_enumerate_devices(uintptr_t fdt_addr, device_callback_t callback, void *context) {
    if (fdt_addr == 0) {
        return -1;
    }

    // Parse FDT header
    fdt_header_t *header = (fdt_header_t *)fdt_addr;

    // Verify magic number
    uint32_t magic = be32_to_cpu(header->magic);
    if (magic != FDT_MAGIC) {
        return -1;
    }

    // Get pointers to FDT sections
    uint32_t struct_offset = be32_to_cpu(header->off_dt_struct);
    uint32_t strings_offset = be32_to_cpu(header->off_dt_strings);
    uint32_t totalsize = be32_to_cpu(header->totalsize);
    uint32_t struct_size = be32_to_cpu(header->size_dt_struct);

    // Validate offsets and sizes
    if (struct_offset > totalsize || strings_offset > totalsize ||
        struct_offset + struct_size > totalsize) {
        return -1;
    }

    uint8_t *fdt_base = (uint8_t *)fdt_addr;
    uint32_t *struct_ptr = (uint32_t *)(fdt_base + struct_offset);
    char *strings_base = (char *)(fdt_base + strings_offset);
    uintptr_t struct_end = fdt_addr + struct_offset + struct_size;

    // State tracking
    int depth = 0;
    int device_count = 0;

    // Current device being built
    device_t current_device = {0};
    bool has_reg = false;
    bool has_compatible = false;

    // Walk the structure block
    while (1) {
        // Safety check before reading token
        if ((uintptr_t)struct_ptr >= struct_end) {
            return device_count;
        }

        uint32_t token = be32_to_cpu(*struct_ptr);
        struct_ptr++;

        switch (token) {
            case FDT_BEGIN_NODE: {
                // Safety check before reading node name
                if ((uintptr_t)struct_ptr >= struct_end) {
                    return device_count;
                }

                // Read node name (null-terminated string)
                char *node_name = (char *)struct_ptr;

                // Calculate safe length for node name (can't exceed remaining space)
                size_t max_name_len = struct_end - (uintptr_t)struct_ptr;
                size_t name_len = 0;
                while (name_len < max_name_len && node_name[name_len] != '\0') {
                    name_len++;
                }

                // If we didn't find null terminator, FDT is corrupted
                if (name_len >= max_name_len) {
                    return device_count;
                }

                // Reset device state
                has_reg = false;
                has_compatible = false;
                current_device = (device_t){0};
                current_device.name = node_name;
                current_device.depth = depth;  // Track tree depth

                // Advance past node name (aligned to 4 bytes)
                struct_ptr = (uint32_t *)((char *)struct_ptr + align_up(name_len + 1));

                // Verify we haven't gone past the end
                if ((uintptr_t)struct_ptr > struct_end) {
                    return device_count;
                }

                depth++;
                break;
            }

            case FDT_END_NODE: {
                // If this node has both reg and compatible, report it as a device
                if (has_reg && has_compatible && callback) {
                    callback(&current_device, context);
                    device_count++;
                }

                depth--;
                break;
            }

            case FDT_PROP: {
                // Safety check before reading property header
                if ((uintptr_t)struct_ptr + sizeof(fdt_prop_t) > struct_end) {
                    return device_count;
                }

                // Read property structure (read fields before advancing pointer)
                uint32_t prop_len_raw = ((uint32_t*)struct_ptr)[0];
                uint32_t prop_nameoff_raw = ((uint32_t*)struct_ptr)[1];
                uint32_t prop_len = be32_to_cpu(prop_len_raw);
                uint32_t prop_nameoff = be32_to_cpu(prop_nameoff_raw);

                // Advance to property data
                uint8_t *prop_data = (uint8_t *)((uint32_t*)struct_ptr + 2);

                // Safety check for property data - ensure it's fully within bounds
                if ((uintptr_t)prop_data + prop_len > struct_end) {
                    return device_count;
                }

                // Get property name from strings block
                char *prop_name = strings_base + prop_nameoff;

                // Update struct_ptr for next iteration
                struct_ptr = (uint32_t *)prop_data;

                // Parse interesting properties
                if (str_equal(prop_name, "reg")) {
                    // reg property contains address/size pairs (cells)
                    // For simplicity, assume #address-cells=2, #size-cells=2
                    if (prop_len >= 16 && (uintptr_t)prop_data + 16 <= struct_end) {
                        // Read 64-bit address (big-endian) safely with unaligned access
                        uint32_t addr_hi = read_be32_unaligned(prop_data);
                        uint32_t addr_lo = read_be32_unaligned(prop_data + 4);
                        uint32_t size_hi = read_be32_unaligned(prop_data + 8);
                        uint32_t size_lo = read_be32_unaligned(prop_data + 12);

                        uint64_t addr = ((uint64_t)addr_hi << 32) | addr_lo;
                        uint64_t size = ((uint64_t)size_hi << 32) | size_lo;

                        current_device.reg_base = addr;
                        current_device.reg_size = size;
                        has_reg = true;
                    }
                } else if (str_equal(prop_name, "compatible")) {
                    // compatible property is a list of null-terminated strings
                    // Use the first one
                    if (prop_len > 0) {
                        current_device.compatible = (const char*)prop_data;
                        has_compatible = true;
                    }
                }

                // Advance past property data (aligned to 4 bytes)
                struct_ptr = (uint32_t *)((char *)struct_ptr + align_up(prop_len));

                // Verify we haven't gone past the end
                if ((uintptr_t)struct_ptr > struct_end) {
                    return device_count;
                }

                break;
            }

            case FDT_NOP:
                // Skip NOP tokens
                break;

            case FDT_END:
                // Reached end of structure block
                return device_count;

            default:
                // Unknown token - FDT might be corrupted
                return device_count;
        }
    }

    return device_count;
}
