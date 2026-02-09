# Logging

YasouOS has a tag-based logging system with four severity levels. Log levels are configurable at boot via kernel command line arguments, both globally and per-tag.

## Log Levels

| Level | Value | Description |
|-------|-------|-------------|
| `LOG_ERROR` | 0 | Errors that prevent correct operation |
| `LOG_WARN` | 1 | Unexpected conditions that may indicate problems |
| `LOG_INFO` | 2 | General operational messages (default) |
| `LOG_DEBUG` | 3 | Verbose output for development and debugging |

## API

### Initialization

```c
void log_init(const char *cmdline);
```

Parses the kernel command line for log level overrides. Must be called once early in boot, before any logging.

### Tag Registration

```c
log_tag_t *log_register(const char *name, log_level_t default_level);
```

Registers a named log tag. Each subsystem (driver, app, kernel module) registers its own tag. Returns `NULL` if the registry is full (max 32 tags). Tags registered after `log_init()` still respect any pending command line overrides.

### Simple Logging

```c
void log_error(const log_tag_t *tag, const char *msg);
void log_warn(const log_tag_t *tag, const char *msg);
void log_info(const log_tag_t *tag, const char *msg);
void log_debug(const log_tag_t *tag, const char *msg);
```

Print a message with prefix. Output format: `[LEVEL][tag] message`

### Structured Logging

For messages that include hex values, addresses, or other formatted data, use `log_enabled()` and `log_prefix()`:

```c
bool log_enabled(const log_tag_t *tag, log_level_t level);
void log_prefix(const log_tag_t *tag, log_level_t level);
```

This avoids computing expensive output when the level is disabled:

```c
if (log_enabled(tag, LOG_DEBUG)) {
    log_prefix(tag, LOG_DEBUG);
    puts("addr=0x");
    put_hex64(addr);
    puts(" len=");
    put_hex32(len);
    puts("\n");
}
```

## Command Line Configuration

Log levels are set via `-append` in QEMU.

### Global Level

Set the level for all tags:

```
-append "log=debug"
-append "log=warn"
```

### Per-Tag Level

Override the level for a specific tag using `log.<tag>=<level>`:

```
-append "log=warn log.virtio-net=debug"
```

This sets the global level to `warn` but enables `debug` output for the `virtio-net` driver only.

### Combining with App Arguments

```
-append "log=debug app=packet-print"
-append "log=warn log.http-hello=debug app=http-hello"
```

## Usage

### Registering a Tag

Each subsystem registers a tag, typically as a file-level static:

```c
static log_tag_t *my_log;

void my_init(void) {
    my_log = log_register("my-module", LOG_INFO);
    log_info(my_log, "Module initialized\n");
}
```

### Example Output

```
[INFO][kernel] Hello World from YasouOS!
[INFO][kernel] Kernel command line: log=debug app=packet-print
[DEBUG][virtio-net] Initializing RX queue...
[DEBUG][virtio-net] RX queue initialized
[INFO][virtio-net] Driver initialized successfully
[INFO][packet-print] MAC: 52:54:00:12:34:56
[INFO][packet-print] Listening for UDP packets on port 5000...
```
