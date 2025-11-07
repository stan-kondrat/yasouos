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
                size_t name_len = strlen(node_name);

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
