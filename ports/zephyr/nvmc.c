/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Daniel Campora on behalf of REMOTE TECH LTD
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>

#include "nvmc.h"


// declare private constants

#define NVS_PARTITION			storage_partition
#define NVS_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(NVS_PARTITION)


// declare private functions


// declare private data

static struct nvs_fs nvmc_fs;


// define public functions

void nvmc_init (void) {

    int rc = 0;
    struct flash_pages_info info;

	/* define the nvs file system by settings with:
	 *	sector_size equal to the pagesize,
	 *	3 sectors
	 *	starting at NVS_PARTITION_OFFSET
	 */
	nvmc_fs.flash_device = NVS_PARTITION_DEVICE;
	if (!device_is_ready(nvmc_fs.flash_device)) {
		printk("Flash device %s is not ready\n", nvmc_fs.flash_device->name);
		return;
	}
	nvmc_fs.offset = NVS_PARTITION_OFFSET;
	rc = flash_get_page_info_by_offs(nvmc_fs.flash_device, nvmc_fs.offset, &info);
	if (rc) {
		printk("Unable to get page info, rc=%d\n", rc);
		return;
	}
	nvmc_fs.sector_size = info.size;
	nvmc_fs.sector_count = 3U;

	rc = nvs_mount(&nvmc_fs);
	if (rc) {
		printk("Flash Init failed, rc=%d\n", rc);
		return;
	}

    uint32_t signature = 0;
    if (!nvmc_get (E_NVM_SIGNATURE, &signature) || signature != NVMC_SIGNAUTRE_VALUE) {
        if (!nvmc_set (E_NVM_SIGNATURE, NVMC_SIGNAUTRE_VALUE)) {
            printk("NVM signature set failed!\n");
        }

        if (!nvmc_set (E_NVM_CHARGE_COMPLETED, false)) {
            printk("NVM charge completed clear failed!\n");
        }

        if (!nvmc_set (E_NVM_BATTERY_EMPTY, false)) {
            printk("NVM charge depleted clear failed!\n");
        }
    }
}

bool nvmc_set (teNvmElement element, uint32_t value) {
    uint16_t id = element + 1;

    if (nvs_write(&nvmc_fs, id, &value, sizeof(value)) == sizeof(value)) {
        return true;
    }
    return false;
}

bool nvmc_get (teNvmElement element, uint32_t *value) {
    uint16_t id = element + 1;

	if (nvs_read(&nvmc_fs, id, value, sizeof(*value)) > 0) {
		return true;
	}
    return false;
}
