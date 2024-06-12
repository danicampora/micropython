
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <time.h>
#include <sys/time.h>

#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "shared/timeutils/timeutils.h"
#include "py/builtin.h"
#include "shared/runtime/pyexec.h"
#include "lib/sdk-nrfxlib/nrf_modem/include/nrf_modem_dect_phy.h"
#include "lib/sdk-nrf/include/modem/nrf_modem_lib.h"
#include "dect.h"


// declare local private functions

// define module exported functions

static mp_obj_t dect_version(void) {
    return mp_obj_new_int(1);
}
static MP_DEFINE_CONST_FUN_OBJ_0(dect_version_obj, dect_version);

static mp_obj_t dect_init(size_t n_args, const mp_obj_t *args) {


    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(dect_init_obj, 3, 3, dect_init);

static mp_obj_t dect_tx(size_t n_args, const mp_obj_t *args) {


    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(dect_tx_obj, 3, 3, dect_tx);

static mp_obj_t dect_rx(size_t n_args, const mp_obj_t *args) {


    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(dect_rx_obj, 3, 3, dect_rx);

static const mp_rom_map_elem_t dect_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),                    MP_ROM_QSTR(MP_QSTR_dect) },

    { MP_ROM_QSTR(MP_QSTR_version),                     MP_ROM_PTR(&dect_version_obj) },
    { MP_ROM_QSTR(MP_QSTR_init),                        MP_ROM_PTR(&dect_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_tx),                          MP_ROM_PTR(&dect_tx_obj) },
    { MP_ROM_QSTR(MP_QSTR_rx),                          MP_ROM_PTR(&dect_rx_obj) },
};

static MP_DEFINE_CONST_DICT(dect_module_globals, dect_module_globals_table);

const mp_obj_module_t dect_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&dect_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_dect, dect_module);


// define local private functions
