// init_apps.c - Application initialization based on kernel command line
#include "../common/common.h"

// Application prototypes
extern void app_illegal_instruction(void);

// Forward declarations
static const char* find_param(const char* cmdline, const char* param);
static int param_has_value(const char* param_pos, const char* value);

void init_apps(const char* cmdline) {
    if (cmdline == NULL) {
        return;
    }

    // Check for app=illegal-instruction
    const char* app_param = find_param(cmdline, "app");
    if (app_param != NULL && param_has_value(app_param, "illegal-instruction")) {
        app_illegal_instruction();
        // Should never return
    }
}

// Helper function to find a parameter in the command line
static const char* find_param(const char* cmdline, const char* param) {
    if (cmdline == NULL || param == NULL) {
        return NULL;
    }

    size_t param_len = strlen(param);
    const char* pos = cmdline;

    while (*pos != '\0') {
        // Skip whitespace
        while (*pos == ' ' || *pos == '\t') {
            pos++;
        }

        // Check if this matches our parameter
        if (strncmp(pos, param, param_len) == 0) {
            // Ensure it's followed by '=' or end of word
            if (pos[param_len] == '=' || pos[param_len] == '\0' ||
                pos[param_len] == ' ' || pos[param_len] == '\t') {
                return pos;
            }
        }

        // Move to next space or end
        while (*pos != '\0' && *pos != ' ' && *pos != '\t') {
            pos++;
        }
    }

    return NULL;
}

// Helper function to check if a parameter has a specific value
static int param_has_value(const char* param_pos, const char* value) {
    if (param_pos == NULL || value == NULL) {
        return 0;
    }

    // Find the '=' sign
    while (*param_pos != '=' && *param_pos != '\0' &&
           *param_pos != ' ' && *param_pos != '\t') {
        param_pos++;
    }

    if (*param_pos != '=') {
        return 0;
    }

    param_pos++; // Skip '='

    size_t value_len = strlen(value);
    if (strncmp(param_pos, value, value_len) == 0) {
        // Check if value ends properly
        char next_char = param_pos[value_len];
        if (next_char == '\0' || next_char == ' ' || next_char == '\t') {
            return 1;
        }
    }

    return 0;
}
