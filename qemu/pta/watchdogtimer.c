/*
 * Watchdog driver for Broadcom BCM2835
 *
 * "bcm2708_wdog" driver written by Luke Diamand that was obtained from
 * branch "rpi-3.6.y" of git://github.com/raspberrypi/linux.git was used
 * as a hardware reference for the Broadcom BCM2835 watchdog timer.
 *
 * Copyright (C) 2013 Lubomir Rintel <lkundrak@v3.sk>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include <compiler.h>
#include <stdio.h>
#include <trace.h>
#include <kernel/pseudo_ta.h>
#include <mm/tee_pager.h>
#include <mm/tee_mm.h>
#include <string.h>
#include <string_ext.h>
#include <malloc.h>

#include <mm/core_mmu.h>
#include <mm/core_memprot.h>
#include <platform_config.h>
#include <io.h>
#include <pta_watchdog_timer.h>
#include <initcall.h>

#define PTA_NAME "watchdogtimer.pta"

#define GPIO_ON             0
#define GPIO_OFF            1
#define PM_START            2

#define RPI3_PERI_BASE      0x3F000000
#define GPIO_BASE           (RPI3_PERI_BASE + 0x200000)

//bcm2835-wdt 3f100000.watchdog: Broadcom BCM2835 watchdog timer
#define WDT_BASE            (RPI3_PERI_BASE +0x100000)
#define PM_RSTC             0x1c
#define PM_WDOG             0x24

#define PM_PASSWORD                 0x5a000000
#define PM_RSTC_WRCFG_FULL_RESET    0x00000020
#define PM_WDOG_TIME_SET            0x000fffff

static vaddr_t nsec_periph_base(paddr_t pa, size_t len) {
    if (cpu_mmu_enabled()) {
        return (vaddr_t)phys_to_virt(pa, MEM_AREA_IO_NSEC);
    }
    return (vaddr_t)pa;
}

static TEE_Result testWDTOn(void) {
    uint32_t data;
    unsigned int timeout = 10;

    DMSG("testWDTOn called");
    vaddr_t wdt_base = nsec_periph_base(WDT_BASE, 1);
    DMSG("va wdt_base %ld", wdt_base);

    //data = io_read32(wdt_base & PM_WDOG_TIME_SET);
    data = io_read32(wdt_base);
    DMSG("data: %ld", data);

    io_write32(wdt_base + PM_WDOG, PM_PASSWORD | (timeout & PM_WDOG_TIME_SET));
    DMSG("WDT initi done");
    io_write32(wdt_base + PM_RSTC, PM_PASSWORD | PM_RSTC_WRCFG_FULL_RESET);

    DMSG("WDT timer reset is complete");
    
    return TEE_SUCCESS;
}

static TEE_Result invoke_command(
	void *psess __unused,
	uint32_t cmd,
	uint32_t ptypes, TEE_Param params[TEE_NUM_PARAMS]
) {
	(void)ptypes;
	(void)params;

	switch (cmd) {
		case GPIO_ON:
		case GPIO_OFF:
        case PM_START:
            return testWDTOn();
        default:
            break;
	}
	return TEE_ERROR_BAD_PARAMETERS;
}

pseudo_ta_register(.uuid = PTA_WATCHDOG_TIMER_UUID, .name = PTA_NAME,
		   .flags = PTA_DEFAULT_FLAGS,
		   .invoke_command_entry_point = invoke_command);