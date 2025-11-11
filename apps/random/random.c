#include "random.h"
#include "../../kernel/devices/devices.h"
#include "../../kernel/resources/resources.h"
#include "../../drivers/virtio_rng/virtio_rng.h"
#include "../../common/common.h"

// ============================================================================
// Random Module State
// ============================================================================

#define MAX_HARDWARE_RNG_DEVICES 4

static struct {
    virtio_rng_t rng_contexts[MAX_HARDWARE_RNG_DEVICES];  // Hardware RNG contexts
    resource_t *resources[MAX_HARDWARE_RNG_DEVICES];      // Associated resources
    int device_count;                                      // Number of acquired devices
    int next_device_index;                                 // Round-robin index for device selection
    bool initialized;                                      // Module initialized
} random_state = {
    .rng_contexts = {{0}},
    .resources = {NULL},
    .device_count = 0,
    .next_device_index = 0,
    .initialized = false
};

// ============================================================================
// Software Fallback: xorshift64 PRNG
// ============================================================================

typedef struct {
    uint64_t state;
} prng_state_t;

static prng_state_t prng_state;

// xorshift64 algorithm - simple, fast, non-cryptographic PRNG
static uint64_t xorshift64(prng_state_t *state) {
    uint64_t x = state->state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state->state = x;
    return x;
}

// Initialize PRNG with seed from device addresses
static void prng_init(void) {
    uint64_t seed = 0x123456789ABCDEF0ULL;  // Base seed

    // Mix in device addresses for entropy
    device_t *device = devices_get_first();
    int device_count = 0;
    while (device != NULL && device_count < 16) {
        seed ^= device->reg_base;
        seed ^= (uint64_t)device->vendor_id << 32;
        seed ^= (uint64_t)device->device_id << 16;
        seed = (seed << 7) | (seed >> 57);  // Rotate
        device = devices_get_next(device);
        device_count++;
    }

    // Ensure seed is never zero
    if (seed == 0) {
        seed = 0x123456789ABCDEF0ULL;
    }

    prng_state.state = seed;
}

// Fill buffer with random bytes from PRNG
static void prng_fill_bytes(uint8_t *buffer, size_t length) {
    while (length >= 8) {
        uint64_t val = xorshift64(&prng_state);
        for (int i = 0; i < 8; i++) {
            buffer[i] = (val >> (i * 8)) & 0xFF;
        }
        buffer += 8;
        length -= 8;
    }

    if (length > 0) {
        uint64_t val = xorshift64(&prng_state);
        for (size_t i = 0; i < length; i++) {
            buffer[i] = (val >> (i * 8)) & 0xFF;
        }
    }
}

// ============================================================================
// Hardware RNG Support
// ============================================================================

static int hardware_read_bytes(virtio_rng_t *ctx,
                               uint8_t *buffer,
                               size_t length) {
    if (!ctx) {
        puts("  virtio-rng: invalid context, falling back to PRNG\n");
        prng_fill_bytes(buffer, length);
        return (int)length;
    }

    int result = virtio_rng_read(ctx, buffer, length);
    if (result == (int)length) {
        return result;
    }

    // Hardware failed, fall back to software
    puts("  virtio-rng read failed, falling back to PRNG\n");
    prng_fill_bytes(buffer, length);
    return (int)length;
}

// ============================================================================
// Public API Implementation
// ============================================================================

int random_hardware_init(void) {
    // Check if we can acquire more devices
    if (random_state.device_count >= MAX_HARDWARE_RNG_DEVICES) {
        puts("Initializing hardware RNG...\n");
        puts("  Maximum hardware RNG devices already acquired\n");
        return -1;
    }

    puts("Initializing hardware RNG...\n");

    // Get driver descriptor and acquire next available resource
    // Resource manager calls init_context automatically
    const driver_t *driver = virtio_rng_get_driver();
    int device_idx = random_state.device_count;
    resource_t *resource = resource_acquire_available(driver, &random_state.rng_contexts[device_idx]);

    if (resource) {
        random_state.resources[device_idx] = resource;
        random_state.device_count++;
        puts("  Hardware RNG acquired (virtio-rng)\n");
        return 0;
    } else {
        puts("  Hardware RNG unavailable, using software PRNG (xorshift64)\n");
        return -1;
    }
}

int random_get_bytes(uint8_t *buffer, size_t length) {
    if (!buffer || length == 0) {
        return -1;
    }

    // Initialize PRNG on first use if not already initialized
    if (!random_state.initialized) {
        prng_init();
        random_state.initialized = true;
    }

    // Use hardware if available (round-robin across devices), otherwise software fallback
    if (random_state.device_count > 0) {
        // Select device using round-robin
        int device_idx = random_state.next_device_index;
        random_state.next_device_index = (random_state.next_device_index + 1) % random_state.device_count;

        return hardware_read_bytes(&random_state.rng_contexts[device_idx], buffer, length);
    } else {
        prng_fill_bytes(buffer, length);
        return (int)length;
    }
}
