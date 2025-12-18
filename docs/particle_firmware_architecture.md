# Particle P2 Firmware Architecture with Bazel + Pigweed

## What We Built

### Directory Structure
```
lib/gpio_mirror/           # Platform-agnostic GPIO logic
├── gpio_mirror.h/cc       # Uses pw::digital_io interfaces
└── test/                  # Host-side unit tests with mocks

lib/particle_gpio/         # Particle backend for pw_digital_io
├── particle_digital_io.h/cc  # Wraps hal_gpio_* functions

firmware/particle_p2/      # Firmware entry point
├── main.cc                # setup()/loop() + GpioMirror instance
├── flash.sh               # Helper script for flashing
└── BUILD.bazel            # cc_binary target

third_party/particle/      # Particle Device OS SDK
├── hdrs/                  # Headers from Device OS 6.3.3
├── stubs/                 # Dynalib stubs and module exports
└── linker/                # Linker scripts
```

### Build Commands
```bash
# Host tests (passes)
bazel test //lib/gpio_mirror/test:gpio_mirror_test

# P2 firmware ELF (builds successfully)
bazel build --config=p2 //firmware/particle_p2:gpio_mirror_firmware.elf

# Flash attempt (fails with CRC error)
bazel run --config=p2 //firmware/particle_p2:flash
```

## Particle Device OS Module System

### How Particle Modules Work

Particle Device OS uses a modular firmware architecture:

1. **Bootloader** - Handles OTA updates, enters DFU mode
2. **System Parts** - Core OS functionality (system-part1, system-part2)
3. **User Part** - Application code (what we're building)

Each module is a separate binary that:
- Lives at a specific flash address
- Has a **module header** (`module_info_t`) at the start
- Contains a **CRC checksum** that's validated at boot
- Declares **dependencies** on other modules (e.g., user-part depends on system-part1)

### Module Header Structure (`module_info_t`)

Located at the start of each module binary:

```c
typedef struct module_info_t {
    uint32_t module_start_address;    // Where module starts in flash
    uint32_t module_end_address;      // Where module ends
    uint8_t  reserved;
    uint8_t  reserved2;
    uint16_t module_version;          // e.g., 1
    uint16_t platform_id;             // 32 for P2
    uint8_t  module_function;         // 5 = MOD_FUNC_USER_PART
    uint8_t  module_index;            // 1 for user-part
    module_dependency_t dependency;   // system-part version required
    module_dependency_t dependency2;
    // ... more fields ...
    uint32_t crc;                     // CRC32 of module contents
} module_info_t;
```

### Dynalib System

Particle uses "dynalib" (dynamic library) tables for linking:
- System parts export functions via dynalib tables
- User part imports these functions at runtime
- Actual function addresses are resolved when module loads

Our stubs provide weak symbols that get overwritten at runtime:
- `hal_gpio_mode()`, `hal_gpio_read()`, `hal_gpio_write()` - GPIO functions
- Various system functions needed by the module

### User Part Entry Points

The system calls these functions in our user module:
- `module_user_pre_init()` - Called before C runtime init (copies .data, zeroes .bss)
- `module_user_init()` - Calls C++ constructors
- `setup()` - User's setup function
- `loop()` - User's main loop (called repeatedly)

## What We've Accomplished

### ✅ Working Components

1. **Platform-agnostic GPIO logic** - `GpioMirror` class using Pigweed interfaces
2. **Host-side unit tests** - Run on development machine, no hardware needed
3. **Particle GPIO backend** - `ParticleDigitalIn/Out` wrapping HAL functions
4. **Cross-compilation** - ELF builds for Cortex-M33
5. **Pigweed assert integration** - `PW_CHECK` works, strings appear in binary
6. **Linker scripts** - Memory layout for P2's RTL8721DM chip
7. **Module structure** - Export tables, entry points, stubs all link

### ❌ Missing Component: CRC Calculation

The firmware builds but fails to flash because:
```
CRC check failed for module gpio_mirror_firmware.bin
```

Particle's flashing tools validate the module CRC before flashing. Our binary has:
- Correct module header structure (in linker script)
- Correct memory layout
- BUT: CRC field is not calculated/filled

## Particle's Build Process

When building with Particle's toolchain, the post-processing step:

1. Compiles source to ELF
2. Generates binary from ELF
3. **Calculates CRC32** of module contents
4. **Patches CRC** into the module header
5. Optionally compresses with LZ4

We're missing steps 3-4.

## Linker Script Analysis

Our linker scripts define:

### `linker.ld` (main script)
- Includes memory definitions from `platform_ram.ld`
- Defines sections: `.text`, `.data`, `.bss`, `.module_info`, etc.
- Places module_info at start of user part flash region

### `module_info.ld`
- Defines `link_module_info_*` symbols
- Creates the module header structure

### Key Addresses (P2)
- User part starts at: `0x08480000` (XIP flash)
- User part RAM at: `0x02200000` (PSRAM)
- Stack size: 8KB (`__STACKSIZE__`)

## Solutions for CRC Issue

### Option 1: Post-Build CRC Tool
Create a tool that:
1. Reads the ELF/binary
2. Locates module_info structure
3. Calculates CRC32 of module content
4. Patches CRC into binary

Particle has this in `device-os/build/module.mk`:
```makefile
$(call module_crc,$(TARGET_BASE).bin)
```

### Option 2: Use Particle's Module Tool
Particle provides `module_info.py` or similar tools in device-os repo that can:
- Parse module binaries
- Calculate and inject CRC
- Validate module structure

### Option 3: Runtime CRC Skip (Development Only)
Some Particle debug builds skip CRC validation, but this isn't suitable for production.

## Symbols in Our Binary

```bash
arm-none-eabi-nm gpio_mirror_firmware.elf | grep -E "(setup|loop|module)"
```

Output shows all required symbols are present:
- `setup`, `loop` - User entry points
- `module_user_init`, `module_user_setup`, `module_user_loop`
- `module_user_pre_init` - In XIP section

## Current Binary Analysis

### Module Info (at 0x08480000)
```
08480000: 00 00 48 08  - module_start = 0x08480000
08480004: 4c 09 48 08  - module_end = 0x0848094c
08480008: 00 00        - reserved
0848000a: 01 00        - version = 1
0848000c: 20 00        - platform_id = 32 (P2) ✓
0848000e: 05           - function = 5 (USER_PART) ✓
0848000f: 01           - index = 1 ✓
08480010: 04 01 9f 18  - dependency: sys-part1 v6303 ✓
08480014: 00 00 00 00  - dependency2: none ✓
```

### Module Info Suffix
Contains placeholder SHA256 hash (01 02 03 04...) and CRC placeholder.

### The CRC Problem

In `module_info.inc` line 167-169:
```c
__attribute__((section(".modinfo.module_info_crc"), used))
const module_info_crc_t module_info_crc = {
    0x12345678  // PLACEHOLDER - must be patched!
};
```

The CRC field has a placeholder value that MUST be calculated and patched post-build.

### Particle's CRC Algorithm

From Device OS source, the CRC is calculated as:
1. CRC32 of module from `module_start` to just before CRC field
2. Using standard CRC-32 polynomial (0x04C11DB7 or reflected)
3. The suffix_size field tells where the CRC is located

## Next Steps

1. **Create CRC calculation tool** - Python script to:
   - Load binary
   - Find module_info and suffix structures
   - Calculate CRC32 of content
   - Patch CRC into binary

2. **Add Bazel genrule** - Post-process ELF → patched BIN

3. **Test flash** - Once CRC is valid, flash should succeed

## References

- Particle Device OS: https://github.com/particle-iot/device-os
- Module system: `device-os/modules/`
- Linker scripts: `device-os/modules/shared/*/linker*.ld`
- CRC code: `device-os/build/arm-tools.mk`, `module.mk`
