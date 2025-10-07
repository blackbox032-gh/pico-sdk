/*
 * Copyright (c) 2025 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/secure.h"
#include "pico/runtime_init.h"

#include "hardware/timer.h"
#include "hardware/irq.h"

#include "hardware/structs/scb.h"
#include "hardware/structs/sau.h"
#include "hardware/structs/m33.h"
#include "hardware/structs/accessctrl.h"

void __attribute__((noreturn)) secure_launch_nonsecure_binary(uint32_t vtor_address, uint32_t stack_limit) {
    uint32_t *vtor = (uint32_t*)vtor_address;
    uint32_t stack_pointer = *(vtor + 0);
    uint32_t entry_point = *(vtor + 1);
    scb_ns_hw->vtor = vtor_address;

    pico_default_asm(
        "msr msp_ns, %0\n"
        "msr msplim_ns, %1\n"
        "movs r1, %2\n"
        "bxns r1"
        :
        :   "r" (stack_pointer),
            "r" (stack_limit),
            "r" (entry_point & ~1)  // make sure thumb bit is clear for blxns
    );

    __builtin_unreachable();
}


void secure_sau_configure_region(uint region, uint32_t base, uint32_t limit, bool enabled, bool nsc) {
    sau_hw->rnr = region;
    sau_hw->rbar = base & M33_SAU_RBAR_BADDR_BITS;
    sau_hw->rlar = ((limit-1) & M33_SAU_RLAR_LADDR_BITS) | (nsc ? M33_SAU_RLAR_NSC_BITS : 0) | (enabled ? M33_SAU_RLAR_ENABLE_BITS : 0);
}


void secure_sau_set_enabled(bool enabled) {
    __dmb();

    if (enabled)
        hw_set_bits(&sau_hw->ctrl, M33_SAU_CTRL_ENABLE_BITS);
    else
        hw_clear_bits(&sau_hw->ctrl, M33_SAU_CTRL_ENABLE_BITS);

    __dsb();
    __isb();
}


#if !PICO_RUNTIME_NO_INIT_NONSECURE_COPROCESSORS
void __weak runtime_init_nonsecure_coprocessors() {
    // Enable NS coprocessor access to anything secure has enabled
    uint32_t cpacr = arm_cpu_hw->cpacr;
    uint32_t nsacr = 0;
    for (int i = 0; i < 16; i++) {
        if (cpacr & (M33_CPACR_CP0_BITS << (i * M33_CPACR_CP1_LSB))) {
            nsacr |= (0x1 << i);
        }
    }
    arm_cpu_hw->nsacr |= nsacr;
}
#endif

#if !PICO_RUNTIME_SKIP_INIT_NONSECURE_COPROCESSORS
PICO_RUNTIME_INIT_FUNC_PER_CORE(runtime_init_nonsecure_coprocessors, PICO_RUNTIME_INIT_NONSECURE_COPROCESSORS);
#endif


#if !PICO_RUNTIME_NO_INIT_NONSECURE_ACCESSCTRL_AND_IRQS
void __weak runtime_init_nonsecure_accessctrl_and_irqs() {
    #if PICO_ALLOW_NONSECURE_DMA
        accessctrl_hw->dma |= 0xacce0000 | ACCESSCTRL_DMA_NSP_BITS | ACCESSCTRL_DMA_NSU_BITS;
    #endif

    #ifdef PICO_ASSIGN_NONSECURE_TIMER
        accessctrl_hw->timer[PICO_ASSIGN_NONSECURE_TIMER] |= 0xacce0000 | ACCESSCTRL_TIMER0_NSP_BITS | ACCESSCTRL_TIMER0_NSU_BITS;

        static_assert(TIMER0_IRQ_0 + 4 == TIMER1_IRQ_0, "Expected 4 IRQs per TIMER");

        irq_assign_to_ns(TIMER0_IRQ_0 + PICO_ASSIGN_NONSECURE_TIMER * 4, true);
        irq_assign_to_ns(TIMER0_IRQ_1 + PICO_ASSIGN_NONSECURE_TIMER * 4, true);
        irq_assign_to_ns(TIMER0_IRQ_2 + PICO_ASSIGN_NONSECURE_TIMER * 4, true);
        irq_assign_to_ns(TIMER0_IRQ_3 + PICO_ASSIGN_NONSECURE_TIMER * 4, true);
    #endif

    #if PICO_ALLOW_NONSECURE_GPIO
        accessctrl_hw->io_bank[0] |= 0xacce0000 | ACCESSCTRL_IO_BANK0_NSP_BITS | ACCESSCTRL_IO_BANK0_NSU_BITS;

        irq_assign_to_ns(IO_IRQ_BANK0_NS, true);
    #endif
}
#endif

#if !PICO_RUNTIME_SKIP_INIT_NONSECURE_ACCESSCTRL_AND_IRQS
PICO_RUNTIME_INIT_FUNC_HW(runtime_init_nonsecure_accessctrl_and_irqs, PICO_RUNTIME_INIT_NONSECURE_ACCESSCTRL_AND_IRQS);
#endif
