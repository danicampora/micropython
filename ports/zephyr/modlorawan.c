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

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <time.h>
#include <sys/time.h>

#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"

#include <zephyr/device.h>
#include <zephyr/lorawan/lorawan.h>
#include <zephyr/kernel.h>

#include "nvmc.h"


#define DEBUG_printf(...)           printk("LoRa: " __VA_ARGS__)

#define LORAWAN_DEV_EUI             { 0xF0, 0x02, 0x20, 0x80, 0x00, 0x47, 0x7E, 0xB8 }
#define LORAWAN_JOIN_EUI            { 0x70, 0xB3, 0xD5, 0x7E, 0xF0, 0x00, 0x3B, 0xFD }
#define LORAWAN_APP_KEY             { 0x3C, 0xB4, 0x03, 0x99, 0x3D, 0xCB, 0x74, 0xFF, 0xD0, 0x5C, 0x01, 0x4A, 0x31, 0x4B, 0xB6, 0x94 }

static void dl_callback(uint8_t port, bool data_pending, int16_t rssi, int8_t snr, uint8_t len, const uint8_t *hex_data);
static void lorwan_datarate_changed(enum lorawan_datarate dr);


static struct lorawan_downlink_cb downlink_cb = {
    .port = LW_RECV_PORT_ANY,
    .cb = dl_callback
};
static const struct device *lora_dev;
static uint16_t modlorawan_dev_nonce;
static bool modlorawan_init_done = false;

// declare local private functions

static void dl_callback(uint8_t port, bool data_pending, int16_t rssi, int8_t snr, uint8_t len, const uint8_t *hex_data) {
    DEBUG_printf("Port %d, Pending %d, RSSI %ddB, SNR %ddBm\n", port, data_pending, rssi, snr);
}

static void lorwan_datarate_changed(enum lorawan_datarate dr) {
    uint8_t unused, max_size;
    lorawan_get_payload_sizes(&unused, &max_size);
    DEBUG_printf("New Datarate: DR_%d, Max Payload %d\n", dr, max_size);
}


// define module exported functions

static mp_obj_t mp_lorawan_init(void) {

    int ret;

    if (!modlorawan_init_done) {

        lora_dev = DEVICE_DT_GET(DT_ALIAS(lora0));
        if (!device_is_ready(lora_dev)) {
            DEBUG_printf("ERROR: %s: device not ready.\n", lora_dev->name);
            mp_raise_OSError(MP_ENODEV);
        }

        /* If more than one region Kconfig is selected, app should set region
            * before calling lorawan_start()
            */
        ret = lorawan_set_region(LORAWAN_REGION_EU868);
        if (ret < 0) {
            DEBUG_printf("ERROR: lorawan_set_region failed: %d", ret);
            mp_raise_OSError(MP_ENOENT);
        }

        ret = lorawan_start();
        if (ret < 0) {
            DEBUG_printf("ERROR: lorawan_start failed: %d\n", ret);
            mp_raise_OSError(MP_EFAULT);
        }

        lorawan_register_downlink_callback(&downlink_cb);
        lorawan_register_dr_changed_callback(lorwan_datarate_changed);

        uint32_t nonce;
        nvmc_get(E_NVM_LORAWAN_DEV_NONCE, &nonce);
        modlorawan_dev_nonce = (uint16_t)nonce;
        DEBUG_printf("Dev Nonce is %d\n", modlorawan_dev_nonce);

        modlorawan_init_done = true;
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(mp_lorawan_init_obj, mp_lorawan_init);

static mp_obj_t mp_lorawan_join(void) {

    struct lorawan_join_config join_cfg;
	uint8_t dev_eui[] = LORAWAN_DEV_EUI;
	uint8_t join_eui[] = LORAWAN_JOIN_EUI;
	uint8_t app_key[] = LORAWAN_APP_KEY;
    int ret;

    join_cfg.mode = LORAWAN_ACT_OTAA;
    join_cfg.dev_eui = dev_eui;
    join_cfg.otaa.join_eui = join_eui;
    join_cfg.otaa.app_key = app_key;
    join_cfg.otaa.nwk_key = app_key;
    join_cfg.otaa.dev_nonce = modlorawan_dev_nonce++;

    if (modlorawan_dev_nonce == 0) {
        modlorawan_dev_nonce = 1;
    }
    nvmc_set(E_NVM_LORAWAN_DEV_NONCE, modlorawan_dev_nonce);

    DEBUG_printf("Joining network over OTAA\n");
    ret = lorawan_join(&join_cfg);
    if (ret < 0) {
        DEBUG_printf("ERROR: lorawan_join_network failed: %d\n", ret);
        mp_raise_OSError(MP_EFAULT);
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(mp_lorawan_join_obj, mp_lorawan_join);

static mp_obj_t mp_lorawan_send(mp_obj_t port_o, mp_obj_t data_o) {

    int32_t port = mp_obj_get_int(port_o);

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(data_o, &bufinfo, MP_BUFFER_READ);

    int ret = lorawan_send(port, bufinfo.buf, bufinfo.len, LORAWAN_MSG_CONFIRMED);

    /*
        * Note: The stack may return -EAGAIN if the provided data
        * length exceeds the maximum possible one for the region and
        * datarate. But since we are just sending the same data here,
        * we'll just continue.
        */
    if (ret == -EAGAIN) {
        DEBUG_printf("lorawan_send busy: %d. Try again...\n", ret);
        return mp_obj_new_int(0);
    }

    if (ret < 0) {
        DEBUG_printf("lorawan_send failed: %d\n", ret);
        return mp_obj_new_int(0);
    }

    return mp_obj_new_int(bufinfo.len);
}
static MP_DEFINE_CONST_FUN_OBJ_2(mp_lorawan_send_obj, mp_lorawan_send);

static const mp_rom_map_elem_t lorawan_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_lorawan) },

    { MP_ROM_QSTR(MP_QSTR_init),                        MP_ROM_PTR(&mp_lorawan_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_join),                        MP_ROM_PTR(&mp_lorawan_join_obj) },
    { MP_ROM_QSTR(MP_QSTR_send),                        MP_ROM_PTR(&mp_lorawan_send_obj) },
};

static MP_DEFINE_CONST_DICT(lorawan_module_globals, lorawan_module_globals_table);

const mp_obj_module_t lorawan_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&lorawan_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_lorawan, lorawan_module);


// define local private functions
