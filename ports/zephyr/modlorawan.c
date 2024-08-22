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


#define DEBUG_printf(...)           printk("LoRa: " __VA_ARGS__)


#define LORAWAN_DEV_EUI             { 0xDD, 0xEE, 0xAA, 0xDD, 0xBB, 0xEE, 0xEE, 0xFF }
#define LORAWAN_JOIN_EUI            { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define LORAWAN_APP_KEY             { 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C }


static void dl_callback(uint8_t port, bool data_pending, int16_t rssi, int8_t snr, uint8_t len, const uint8_t *hex_data);
static void lorwan_datarate_changed(enum lorawan_datarate dr);


static struct lorawan_downlink_cb downlink_cb = {
    .port = LW_RECV_PORT_ANY,
    .cb = dl_callback
};
static const struct device *lora_dev;

// declare local private functions

static void dl_callback(uint8_t port, bool data_pending, int16_t rssi, int8_t snr, uint8_t len, const uint8_t *hex_data) {
    DEBUG_printf("Port %d, Pending %d, RSSI %ddB, SNR %ddBm", port, data_pending, rssi, snr);
}

static void lorwan_datarate_changed(enum lorawan_datarate dr) {
    uint8_t unused, max_size;
    lorawan_get_payload_sizes(&unused, &max_size);
    DEBUG_printf("New Datarate: DR_%d, Max Payload %d", dr, max_size);
}


// define module exported functions

static mp_obj_t lorawan_version(void) {
    return mp_obj_new_int(1);
}
static MP_DEFINE_CONST_FUN_OBJ_0(lorawan_version_obj, lorawan_version);

static mp_obj_t lorawan_init(void) {

    int ret;

    if (!device_is_ready(lora_dev)) {
        DEBUG_printf("ERROR: %s: device not ready.", lora_dev->name);
        mp_raise_OSError(MP_ENODEV);
    }

#if defined(CONFIG_LORAMAC_REGION_EU868)
    /* If more than one region Kconfig is selected, app should set region
        * before calling lorawan_start()
        */
    ret = lorawan_set_region(LORAWAN_REGION_EU868);
    if (ret < 0) {
        DEBUG_printf("ERROR: lorawan_set_region failed: %d", ret);
        mp_raise_OSError(MP_ENOENT);
    }
    #endif

    ret = lorawan_start();
    if (ret < 0) {
        DEBUG_printf("ERROR: lorawan_start failed: %d", ret);
        mp_raise_OSError(MP_EFAULT);
    }

    lorawan_register_downlink_callback(&downlink_cb);
    lorawan_register_dr_changed_callback(lorwan_datarate_changed);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(lorawan_init_obj, lorawan_init);

static mp_obj_t lorawan_join(void) {

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
    join_cfg.otaa.dev_nonce = 0u;

    DEBUG_printf("Joining network over OTAA");
    ret = lorawan_join(&join_cfg);
    if (ret < 0) {
        DEBUG_printf("ERROR: lorawan_join_network failed: %d", ret);
        mp_raise_OSError(MP_EFAULT);
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(lorawan_join_obj, lorawan_join);

static mp_obj_t lorawan_send(mp_obj_t port_o, mp_obj_t data_o) {

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
        DEBUG_printf("lorawan_send busy: %d. Try again...", ret);
        return mp_obj_new_int(0);
    }

    if (ret < 0) {
        DEBUG_printf("lorawan_send failed: %d", ret);
        return mp_obj_new_int(0);
    }

    return mp_obj_new_int(bufinfo.len);
}
static MP_DEFINE_CONST_FUN_OBJ_2(lorawan_send_obj, lorawan_send);

static const mp_rom_map_elem_t lorawan_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_lorawan) },

    { MP_ROM_QSTR(MP_QSTR_version),                     MP_ROM_PTR(&lorawan_version_obj) },
    { MP_ROM_QSTR(MP_QSTR_init),                        MP_ROM_PTR(&lorawan_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_join),                        MP_ROM_PTR(&lorawan_join_obj) },
    { MP_ROM_QSTR(MP_QSTR_send),                        MP_ROM_PTR(&lorawan_send_obj) },
};

static MP_DEFINE_CONST_DICT(lorawan_module_globals, lorawan_module_globals_table);

const mp_obj_module_t lorawan_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&lorawan_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_lorawan, lorawan_module);


// define local private functions
