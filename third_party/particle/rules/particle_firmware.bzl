"""Bazel rules for building Particle P2 firmware."""

load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@pigweed//pw_build:binary_tools.bzl", "pw_elf_to_bin")

# Platform: P2 (RTL8721DM / Cortex-M33)
PARTICLE_P2_PLATFORM_ID = 32
PARTICLE_P2_PLATFORM_NAME = "p2"
PARTICLE_P2_PLATFORM_GEN = 3

# Device OS version
DEVICE_OS_VERSION = 6304  # 6.3.4

def particle_cc_binary(
        name,
        srcs = [],
        deps = [],
        copts = [],
        defines = [],
        linkopts = [],
        **kwargs):
    """Creates a Particle P2 firmware binary.

    This macro wraps cc_binary with all the Particle-specific compiler flags,
    linker scripts, and dependencies needed to build a user firmware module.

    Args:
        name: Target name. Should end with .elf for clarity.
        srcs: Source files for the firmware.
        deps: Dependencies (user libraries).
        copts: Additional compiler flags.
        defines: Additional preprocessor defines.
        linkopts: Additional linker flags.
        **kwargs: Additional arguments passed to cc_binary.
    """

    # Particle-specific compiler flags
    particle_copts = [
        "-flto",
        "-ffat-lto-objects",
        # Relax warnings for Device OS code
        "-Wno-redundant-decls",
        "-Wno-switch",
        "-Wno-unused-parameter",
        "-Wno-cast-qual",
        "-Wno-missing-field-initializers",
    ]

    # Particle-specific linker flags
    particle_linkopts = [
        "-Tthird_party/particle/device-os/modules/tron/user-part/linker.ld",
        "-Lthird_party/particle/rules",  # For memory_platform_user.ld
        "-Lthird_party/particle/device-os/modules/tron/user-part",
        "-Lthird_party/particle/device-os/modules/tron/system-part1",
        "-Lthird_party/particle/device-os/modules/shared/rtl872x",
        "-Lthird_party/particle/device-os/build/arm/linker",
        "-Lthird_party/particle/device-os/build/arm/linker/rtl872x",
        "-Wl,--defsym,__STACKSIZE__=8192",
        "-Wl,--defsym,__STACK_SIZE=8192",
        # Force linker to include user entry points
        "-Wl,--undefined=setup",
        "-Wl,--undefined=loop",
        "-Wl,--undefined=module_user_init_hook",
        "-Wl,--undefined=_post_loop",
        "-Wl,--gc-sections",
        "-nostartfiles",
        "-fno-lto",  # Disable LTO at link time (as per Particle build)
    ]

    cc_binary(
        name = name,
        srcs = srcs,
        deps = deps + [
            "//third_party/particle:device_os_user_part",
            "@pigweed//pw_assert_basic",  # pw_assert implementation
        ],
        copts = particle_copts + copts,
        defines = defines,
        additional_linker_inputs = [
            "//third_party/particle:linker_scripts",
        ],
        linkopts = particle_linkopts + linkopts,
        target_compatible_with = ["@pigweed//pw_build/constraints/arm:cortex-m33"],
        **kwargs
    )

def particle_firmware_binary(
        name,
        elf,
        **kwargs):
    """Creates a flashable .bin from an ELF with SHA256/CRC32 patched.

    Args:
        name: Target name for the .bin file.
        elf: Label of the ELF file to convert.
        **kwargs: Additional arguments passed to genrule.
    """
    # Use Pigweed's pw_elf_to_bin for proper toolchain integration
    bin_name = name + "_raw"
    pw_elf_to_bin(
        name = bin_name,
        elf_input = elf,
        bin_out = bin_name + ".bin",
    )

    # Patch SHA256 and CRC32 into the binary
    native.genrule(
        name = name,
        srcs = [":" + bin_name + ".bin"],
        outs = [name + ".bin"],
        cmd = """
            cp $< $@
            chmod +w $@
            python3 $(location //tools:particle_crc) $@
        """,
        tools = ["//tools:particle_crc"],
        target_compatible_with = ["@pigweed//pw_build/constraints/arm:cortex-m33"],
        **kwargs
    )

def particle_flash_binary(
        name,
        firmware,
        **kwargs):
    """Creates a target to flash firmware to a Particle device.

    Args:
        name: Target name.
        firmware: Label of the firmware binary (.bin or .elf) to flash.
        **kwargs: Additional arguments passed to sh_binary.
    """
    native.sh_binary(
        name = name,
        srcs = ["//third_party/particle/rules:flash.sh"],
        data = [firmware],
        args = ["$(location " + firmware + ")"],
        tags = ["local"],  # Bypass sandbox to access particle credentials
        **kwargs
    )
