// Minimal pw_assert stub for Particle P2 firmware
// This provides the pw_assert_basic_HandleFailure function without
// depending on pw_sys_io.

#include <cstdlib>

extern "C" {

// This function is called when an assert fails in Pigweed code
void pw_assert_basic_HandleFailure(const char* /* file */,
                                    int /* line */,
                                    const char* /* function */,
                                    const char* /* message */) {
    // In production, we might want to:
    // - Log the failure to flash
    // - Trigger a watchdog reset
    // - Enter safe mode
    // For now, just abort (this will trigger a hard fault on bare metal)
    while (1) {
        // Infinite loop - will trigger watchdog if enabled
        __asm volatile("bkpt #0");  // Breakpoint instruction for debugging
    }
}

}  // extern "C"
