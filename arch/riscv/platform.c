#include "common.h"

// SBI (Supervisor Binary Interface) call
struct sbiret {
    long error;
    long value;
};

static struct sbiret sbi_call(long arg0, long arg1, long arg2, long arg3,
                              long arg4, long arg5, long fid, long eid) {
    register long a0 __asm__("a0") = arg0;
    register long a1 __asm__("a1") = arg1;
    register long a2 __asm__("a2") = arg2;
    register long a3 __asm__("a3") = arg3;
    register long a4 __asm__("a4") = arg4;
    register long a5 __asm__("a5") = arg5;
    register long a6 __asm__("a6") = fid;
    register long a7 __asm__("a7") = eid;

    __asm__ __volatile__("ecall"
                         : "=r"(a0), "=r"(a1)
                         : "r"(a0), "r"(a1), "r"(a2), "r"(a3), "r"(a4), "r"(a5),
                           "r"(a6), "r"(a7)
                         : "memory");
    return (struct sbiret){.error = a0, .value = a1};
}

void platform_init(void) {
    // SBI/OpenSBI already initialized
}

void platform_putchar(char ch) {
    // SBI Console Putchar (EID #0x01)
    sbi_call(ch, 0, 0, 0, 0, 0, 0, 1);
}

void platform_puts(const char* str) {
    while (*str) {
        platform_putchar(*str++);
    }
}

void platform_halt(void) {
    // SBI System Reset Extension (EID 0x53525354 "SRST")
    // Function ID 0x00000000 (sbi_system_reset)
    // arg0 = reset_type (0x00000000 = shutdown)
    // arg1 = reset_reason (0x00000000 = no reason)
    sbi_call(0x00000000, 0x00000000, 0, 0, 0, 0, 0x00000000, 0x53525354);

    // Fallback in case SBI shutdown fails
    for (;;) {
        __asm__ __volatile__("wfi");
    }
}