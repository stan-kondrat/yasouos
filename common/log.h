#pragma once

#include "types.h"

// Log levels (lower = more important)
typedef enum {
    LOG_ERROR = 0,
    LOG_WARN  = 1,
    LOG_INFO  = 2,
    LOG_DEBUG = 3
} log_level_t;

// Log tag handle
typedef struct log_tag log_tag_t;

// Initialize logging system, parse cmdline for level overrides.
// Format: "log=<level>" for global, "log.<tag>=<level>" for per-tag.
// Example: "log=warn log.virtio-net=debug"
void log_init(const char *cmdline);

// Register a named tag with a default level. Returns NULL if registry is full.
log_tag_t *log_register(const char *name, log_level_t default_level);

// Check if a message at the given level would be printed for this tag.
bool log_enabled(const log_tag_t *tag, log_level_t level);

// Print the "[LEVEL][tag] " prefix for a message.
void log_prefix(const log_tag_t *tag, log_level_t level);

// Convenience: print a simple string message at the given level.
void log_error(const log_tag_t *tag, const char *msg);
void log_warn(const log_tag_t *tag, const char *msg);
void log_info(const log_tag_t *tag, const char *msg);
void log_debug(const log_tag_t *tag, const char *msg);
