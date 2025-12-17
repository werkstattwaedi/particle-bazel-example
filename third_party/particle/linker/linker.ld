/* Modified for Bazel build - uses default memory sizes from platform_ram.ld/platform_flash.ld */
/* Original: INCLUDE memory_platform_user.ld (generated at build time, removed) */

INCLUDE platform_ram.ld
INCLUDE platform_flash.ld

MEMORY
{
    INCLUDE memory_backup_ram.ld
    
    APP_FLASH   (rx)  : ORIGIN = platform_user_part_flash_start,           LENGTH = platform_user_part_flash_size
    SRAM        (rwx) : ORIGIN = platform_user_part_static_ram_start,      LENGTH = platform_user_part_static_ram_size
    SECURE_SRAM (rwx) : ORIGIN = platform_user_part_secure_ram_start,      LENGTH = platform_user_part_secure_ram_size
    PSRAM       (rwx) : ORIGIN = platform_user_part_psram_start,           LENGTH = platform_user_part_psram_size
}

/* Define the static locations of the part1 module so the dynamic invoke functions have the base address of each library table. */
INCLUDE module_system_part1_export.ld

/* Definitions for the static parts of the user module */
INCLUDE module_user_export.ld

INCLUDE linker_user_part_common.ld
