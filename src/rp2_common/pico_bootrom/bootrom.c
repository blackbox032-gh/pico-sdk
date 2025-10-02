/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/bootrom.h"
#include "boot/picoboot.h"
#include "boot/picobin.h"
#if !PICO_RP2040
#include "hardware/rcp.h"
#endif
#include "pico/runtime_init.h"

/// \tag::table_lookup[]

void *rom_func_lookup(uint32_t code) {
    return rom_func_lookup_inline(code);
}

void *rom_data_lookup(uint32_t code) {
    return rom_data_lookup_inline(code);
}
/// \end::table_lookup[]

bool rom_funcs_lookup(uint32_t *table, unsigned int count) {
    bool ok = true;
    for (unsigned int i = 0; i < count; i++) {
        table[i] = (uintptr_t) rom_func_lookup(table[i]);
        if (!table[i]) ok = false;
    }
    return ok;
}


void __attribute__((noreturn)) rom_reset_usb_boot(uint32_t usb_activity_gpio_pin_mask, uint32_t disable_interface_mask) {
#ifdef ROM_FUNC_RESET_USB_BOOT
    rom_reset_usb_boot_fn func = (rom_reset_usb_boot_fn) rom_func_lookup(ROM_FUNC_RESET_USB_BOOT);
    func(usb_activity_gpio_pin_mask, disable_interface_mask);
#elif defined(ROM_FUNC_REBOOT)
    uint32_t flags = disable_interface_mask;
    if (usb_activity_gpio_pin_mask) {
        flags |= BOOTSEL_FLAG_GPIO_PIN_SPECIFIED;
        // the parameter is actually the gpio number, but we only care if BOOTSEL_FLAG_GPIO_PIN_SPECIFIED
        usb_activity_gpio_pin_mask = (uint32_t)__builtin_ctz(usb_activity_gpio_pin_mask);
    }
    rom_reboot(REBOOT2_FLAG_REBOOT_TYPE_BOOTSEL | REBOOT2_FLAG_NO_RETURN_ON_SUCCESS, 10, flags, usb_activity_gpio_pin_mask);
    __builtin_unreachable();
#else
    panic_unsupported();
#endif
}

void __attribute__((noreturn)) rom_reset_usb_boot_extra(int usb_activity_gpio_pin, uint32_t disable_interface_mask, bool usb_activity_gpio_pin_active_low) {
#ifdef ROM_FUNC_RESET_USB_BOOT
    (void)usb_activity_gpio_pin_active_low;
    rom_reset_usb_boot_fn func = (rom_reset_usb_boot_fn) rom_func_lookup(ROM_FUNC_RESET_USB_BOOT);
    func(usb_activity_gpio_pin < 0 ? 0 : (1u << usb_activity_gpio_pin), disable_interface_mask);
#elif defined(ROM_FUNC_REBOOT)
    uint32_t flags = disable_interface_mask;
    if (usb_activity_gpio_pin >= 0) {
        flags |= BOOTSEL_FLAG_GPIO_PIN_SPECIFIED;
        if (usb_activity_gpio_pin_active_low) {
            flags |= BOOTSEL_FLAG_GPIO_PIN_ACTIVE_LOW;
        }
    }
    rom_reboot(REBOOT2_FLAG_REBOOT_TYPE_BOOTSEL | REBOOT2_FLAG_NO_RETURN_ON_SUCCESS, 10, flags, (uint)usb_activity_gpio_pin);
    __builtin_unreachable();
#else
    panic_unsupported();
#endif
}

#if !PICO_RP2040
bool rom_get_boot_random(uint32_t out[4]) {
    uint32_t result[5];
    rom_get_sys_info_fn func = (rom_get_sys_info_fn) rom_func_lookup_inline(ROM_FUNC_GET_SYS_INFO);
    if (5 == func(result, count_of(result), SYS_INFO_BOOT_RANDOM)) {
        for(uint i=0;i<4;i++) {
            out[i] = result[i+1];
        }
        return true;
    }
    return false;
}

int rom_add_flash_runtime_partition(uint32_t start_offset, uint32_t size, uint32_t permissions) {
    if ((start_offset) & 4095 || (size & 4095)) return PICO_ERROR_BAD_ALIGNMENT;
    if (!size || start_offset + size > 32 * 1024 * 1024) return PICO_ERROR_INVALID_ARG;
    if (permissions & ~PICOBIN_PARTITION_PERMISSIONS_BITS) return PICO_ERROR_INVALID_ARG;

    void **ptr = (void **)rom_data_lookup(ROM_DATA_PARTITION_TABLE_PTR);
    assert(ptr);
    assert(*ptr);
    struct pt {
        struct {
            uint8_t partition_count;
            uint8_t permission_partition_count; // >= partition_count and includes any regions added at runtime
            bool loaded;
        };
        uint32_t unpartitioned_space_permissions_and_flags;
        resident_partition_t partitions[PARTITION_TABLE_MAX_PARTITIONS];
    } *pt = (struct pt *)*ptr;
    assert(pt->loaded); // even if empty it should have been populated by the bootrom
    if (pt->permission_partition_count < pt->partition_count) pt->permission_partition_count = pt->partition_count;
    if (pt->permission_partition_count < PARTITION_TABLE_MAX_PARTITIONS) {
        pt->partitions[pt->permission_partition_count].permissions_and_location = permissions |
                ((start_offset / 4096) << PICOBIN_PARTITION_LOCATION_FIRST_SECTOR_LSB) |
                ((start_offset + size - 4096) / 4096) << PICOBIN_PARTITION_LOCATION_LAST_SECTOR_LSB;
        pt->partitions[pt->permission_partition_count].permissions_and_flags = permissions;
        return pt->permission_partition_count++;
    }
    return PICO_ERROR_INSUFFICIENT_RESOURCES;
}

int rom_pick_ab_partition_during_update(uint32_t *workarea_base, uint32_t workarea_size, uint partition_a_num) {
#if !PICO_RP2040
    // Generated from adding the following code into the bootrom
    // scan_workarea_t* scan_workarea = (scan_workarea_t*)workarea;
    // printf("VERSION_DOWNGRADE_ERASE_ADDR %08x\n", &(always->zero_init.version_downgrade_erase_flash_addr));
    // printf("TBYB_FLAG_ADDR %08x\n", &(always->zero_init.tbyb_flag_flash_addr));
    // printf("IMAGE_DEF_VERIFIED %08x\n", (uint32_t)&(scan_workarea->parsed_block_loops[0].image_def.core.verified) - (uint32_t)scan_workarea);
    // printf("IMAGE_DEF_TBYB_FLAGGED %08x\n", (uint32_t)&(scan_workarea->parsed_block_loops[0].image_def.core.tbyb_flagged) - (uint32_t)scan_workarea);
    // printf("IMAGE_DEF_BASE %08x\n", (uint32_t)&(scan_workarea->parsed_block_loops[0].image_def.core.enclosing_window.base) - (uint32_t)scan_workarea);
    // printf("IMAGE_DEF_REL_BLOCK_OFFSET %08x\n", (uint32_t)&(scan_workarea->parsed_block_loops[0].image_def.core.window_rel_block_offset) - (uint32_t)scan_workarea);
    #define VERSION_DOWNGRADE_ERASE_ADDR *(uint32_t*)0x400e0338
    #define TBYB_FLAG_ADDR *(uint32_t*)0x400e0348
    #define IMAGE_DEF_VERIFIED(scan_workarea) *(uint32_t*)(0x64 + (uint32_t)scan_workarea)
    #define IMAGE_DEF_TBYB_FLAGGED(scan_workarea) *(bool*)(0x4c + (uint32_t)scan_workarea)
    #define IMAGE_DEF_BASE(scan_workarea) *(uint32_t*)(0x54 + (uint32_t)scan_workarea)
    #define IMAGE_DEF_REL_BLOCK_OFFSET(scan_workarea) *(uint32_t*)(0x5c + (uint32_t)scan_workarea)
#else
    // Prevent linting errors
    #define VERSION_DOWNGRADE_ERASE_ADDR *(uint32_t*)NULL
    #define TBYB_FLAG_ADDR *(uint32_t*)NULL
    #define IMAGE_DEF_VERIFIED(scan_workarea) *(uint32_t*)(NULL + (uint32_t)scan_workarea)
    #define IMAGE_DEF_TBYB_FLAGGED(scan_workarea) *(bool*)(NULL + (uint32_t)scan_workarea)
    #define IMAGE_DEF_BASE(scan_workarea) *(uint32_t*)(NULL + (uint32_t)scan_workarea)
    #define IMAGE_DEF_REL_BLOCK_OFFSET(scan_workarea) *(uint32_t*)(NULL + (uint32_t)scan_workarea)

    panic_unsupported();
#endif

    uint32_t flash_update_base = 0;
    bool tbyb_boot = false;
    uint32_t saved_erase_addr = 0;
    if (rom_get_last_boot_type() == BOOT_TYPE_FLASH_UPDATE) {
        // For a flash update boot, get the flash update base
        boot_info_t boot_info = {};
        int ret = rom_get_boot_info(&boot_info);
        if (ret) {
            flash_update_base = boot_info.reboot_params[0];
            if (boot_info.tbyb_and_update_info & BOOT_TBYB_AND_UPDATE_FLAG_BUY_PENDING) {
                // A buy is pending, so the main software has not been bought
                tbyb_boot = true;
                // Save the erase address, as this will be overwritten by rom_pick_ab_partition
                saved_erase_addr = VERSION_DOWNGRADE_ERASE_ADDR;
            }
        }
    }

    int rc = rom_pick_ab_partition((uint8_t*)workarea_base, workarea_size, partition_a_num, flash_update_base);

    if (!rcp_is_true(IMAGE_DEF_VERIFIED(workarea_base))) {
        // Chosen partition failed verification
        return BOOTROM_ERROR_NOT_FOUND;
    }

    if (IMAGE_DEF_TBYB_FLAGGED(workarea_base)) {
        // The chosen partition is TBYB
        if (tbyb_boot) {
            // The boot partition is also TBYB - cannot update both, so prioritise boot partition
            // Restore the erase address saved earlier
            VERSION_DOWNGRADE_ERASE_ADDR = saved_erase_addr;
            return BOOTROM_ERROR_NOT_PERMITTED;
        } else {
            // Update the tbyb flash address, so that explicit_buy will clear the flag for the chosen partition
            TBYB_FLAG_ADDR =
                    IMAGE_DEF_BASE(workarea_base)
                    + IMAGE_DEF_REL_BLOCK_OFFSET(workarea_base) + 4;
        }
    } else {
        // The chosen partition is not TBYB
        if (tbyb_boot && saved_erase_addr) {
            // The boot partition was TBYB, and requires an erase
            if (VERSION_DOWNGRADE_ERASE_ADDR) {
                // But both the chosen partition requires an erase too
                // As before, prioritise the boot partition, and restore it's saved erase_address
                VERSION_DOWNGRADE_ERASE_ADDR = saved_erase_addr;
                return BOOTROM_ERROR_NOT_PERMITTED;
            } else {
                // The chosen partition doesn't require an erase, so we're fine
                VERSION_DOWNGRADE_ERASE_ADDR = saved_erase_addr;
            }
        }
    }

    return rc;
}

#if PICO_SECURE || PICO_NONSECURE
int __noinline rom_secure_call(uint a, uint b, uint c, uint d, uint func) {
    uint32_t secure_call = (uintptr_t)rom_func_lookup_inline(ROM_FUNC_SECURE_CALL);
    register uint32_t r0 asm("r0") = a;
    register uint32_t r1 asm("r1") = b;
    register uint32_t r2 asm("r2") = c;
    register uint32_t r3 asm("r3") = d;
    register uint32_t r4 asm("r4") = func;
    pico_default_asm_volatile(
            "push {lr}\n"
            "blx %0\n"
            "pop {lr}\n"
            : : "r" (secure_call), "r"(r0), "r"(r1), "r"(r2), "r"(r3), "r"(r4));
    return (int)r0;
}

#if PICO_ALLOW_NONSECURE_STDIO
#include "pico/stdio/driver.h"

#if PICO_NONSECURE
static void stdio_nonsecure_out_chars(const char *buf, int length) {
    rom_secure_call((uint32_t)buf, length, 0, 0, BOOTROM_API_CALLBACK_stdio_out_chars);
}

int stdio_nonsecure_in_chars(char *buf, int length) {
    return PICO_ERROR_NO_DATA;
}

static void stdio_nonsecure_out_flush(void) {}


stdio_driver_t stdio_nonsecure = {
    .out_chars = stdio_nonsecure_out_chars,
    .out_flush = stdio_nonsecure_out_flush,
    .in_chars = stdio_nonsecure_in_chars,
#if PICO_STDIO_ENABLE_CRLF_SUPPORT
    .crlf_enabled = false, // CRLF is handled by the secure side
#endif
};

#if !PICO_RUNTIME_NO_INIT_NONSECURE_STDIO
void __weak runtime_init_nonsecure_stdio() {
    stdio_set_driver_enabled(&stdio_nonsecure, true);
}
#endif

#if !PICO_RUNTIME_SKIP_INIT_NONSECURE_STDIO
PICO_RUNTIME_INIT_FUNC_RUNTIME(runtime_init_nonsecure_stdio, PICO_RUNTIME_INIT_NONSECURE_STDIO);
#endif

#endif // PICO_NONSECURE
#endif // PICO_ALLOW_NONSECURE_STDIO

#if PICO_ALLOW_NONSECURE_RAND
#include "pico/rand.h"

#if PICO_NONSECURE
// override the weak definition
uint64_t get_rand_64(void) {
    return rom_secure_call(0, 0, 0, 0, BOOTROM_API_CALLBACK_get_rand_64);
}
#endif
#endif // PICO_ALLOW_NONSECURE_RAND

#if PICO_ALLOW_NONSECURE_DMA
#include "hardware/dma.h"

#if PICO_SECURE
static int dma_allocate_unused_channel_for_nonsecure(void) {
    int chan = dma_claim_unused_channel(false);
    if (chan < 0) return chan;
    if (chan > PICO_NONSECURE_DMA_MAX_CHANNEL) {
        dma_channel_unclaim(chan);
        return -1;
    }
    hw_clear_bits(&dma_hw->seccfg_ch[chan], DMA_SECCFG_CH0_S_BITS | DMA_SECCFG_CH0_LOCK_BITS);
    return chan;
}
#elif PICO_NONSECURE
int dma_request_unused_channels_from_secure(int num_channels) {
    int i;
    for (i = 0; i < num_channels; i++) {
        int chan = rom_secure_call(0, 0, 0, 0, BOOTROM_API_CALLBACK_dma_allocate_unused_channel_for_nonsecure);
        if (chan < 0) break;
        dma_channel_unclaim(chan);
    }
    return i;
}
#endif
#endif // PICO_ALLOW_NONSECURE_DMA

#if PICO_ALLOW_NONSECURE_USER_IRQ
#include "hardware/irq.h"

#if PICO_SECURE
static int user_irq_claim_unused_for_nonsecure() {
    int bit = user_irq_claim_unused(false);
    if (bit < 0) return bit;
    irq_assign_to_ns(bit, true);
    return bit;
}
#elif PICO_NONSECURE
int user_irq_request_unused_from_secure(int num_irqs) {
    int i;
    for (i = 0; i < num_irqs; i++) {
        int irq = rom_secure_call(0, 0, 0, 0, BOOTROM_API_CALLBACK_user_irq_claim_unused_for_nonsecure);
        if (irq < 0) break;
        user_irq_unclaim(irq);
    }
    return i;
}
#endif

#endif // PICO_ALLOW_NONSECURE_USER_IRQ

#if !PICO_RUNTIME_NO_INIT_BOOTROM_API_CALLBACK
#include <stdio.h>
#include "hardware/clocks.h"

int rom_default_callback(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t fn) {
    switch (fn) {
    #if PICO_ALLOW_NONSECURE_STDIO
        case BOOTROM_API_CALLBACK_stdio_out_chars: {
            stdio_put_string((char*)a, b, false, true);
            stdio_flush();
            return 0;
        }
    #endif
    #if PICO_ALLOW_NONSECURE_RAND
        case BOOTROM_API_CALLBACK_get_rand_64: {
            return get_rand_64();
        }
    #endif
    #if PICO_ALLOW_NONSECURE_DMA
        case BOOTROM_API_CALLBACK_dma_allocate_unused_channel_for_nonsecure: {
            return dma_allocate_unused_channel_for_nonsecure();
        }
    #endif
    #if PICO_ALLOW_NONSECURE_USER_IRQ
        case BOOTROM_API_CALLBACK_user_irq_claim_unused_for_nonsecure: {
            return user_irq_claim_unused_for_nonsecure();
        }
    #endif
        case BOOTROM_API_CALLBACK_clock_get_hz: {
            return clock_get_hz(a);
        }
        default: {
            printf("%d is not a supported rom function\n", fn);
            return BOOTROM_ERROR_INVALID_ARG;
        }

    }
}

static int __attribute__((naked)) rom_default_asm_callback() {
    pico_default_asm_volatile(
        "push {r0, lr}\n"
        "str r4, [sp]\n"
        "bl rom_default_callback\n"
        "pop {r1, pc}\n"
    );
}

void __weak runtime_init_rom_set_default_callback() {
    rom_set_rom_callback(BOOTROM_API_CALLBACK_secure_call, (bootrom_api_callback_generic_t) rom_default_asm_callback);
}
#endif // !PICO_RUNTIME_NO_INIT_BOOTROM_API_CALLBACK

#if !PICO_RUNTIME_SKIP_INIT_BOOTROM_API_CALLBACK
PICO_RUNTIME_INIT_FUNC_RUNTIME(runtime_init_rom_set_default_callback, PICO_RUNTIME_INIT_BOOTROM_API_CALLBACK);
#endif

#if !PICO_RUNTIME_NO_INIT_NONSECURE_CLAIMS
void __weak runtime_init_nonsecure_claims() {
    for(uint i = 0; i < NUM_DMA_CHANNELS; i++) {
        dma_channel_claim(i);
    }

    for (uint i = 0; i < NUM_USER_IRQS; i++) {
        user_irq_claim(FIRST_USER_IRQ + i);
    }
}
#endif

#if !PICO_RUNTIME_SKIP_INIT_NONSECURE_CLAIMS
PICO_RUNTIME_INIT_FUNC_RUNTIME(runtime_init_nonsecure_claims, PICO_RUNTIME_INIT_NONSECURE_CLAIMS);
#endif

#if PICO_NONSECURE && !PICO_RUNTIME_NO_INIT_CLOCKS
#include "hardware/clocks.h"

void runtime_init_clocks() {
    // Set all clocks to the reported frequency from the secure side
    for (uint i = 0; i < CLK_COUNT; i++) {
        uint32_t hz = rom_secure_call(i, 0, 0, 0, BOOTROM_API_CALLBACK_clock_get_hz);
        clock_set_reported_hz(i, hz);
    }
}
#endif

#endif // PICO_SECURE || PICO_NONSECURE

#endif // !PICO_RP2040
