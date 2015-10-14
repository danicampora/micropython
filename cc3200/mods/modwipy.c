#include "py/mpconfig.h"
#include MICROPY_HAL_H
#include "py/obj.h"
#include "py/runtime.h"
#include "mperror.h"


/******************************************************************************/
// Micro Python bindings

STATIC const mp_map_elem_t mp_module_wipy_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__),        MP_OBJ_NEW_QSTR(MP_QSTR_wipy) },
    { MP_OBJ_NEW_QSTR(MP_QSTR_heartbeat),       (mp_obj_t)&mod_wipy_heartbeat_obj },
	{ MP_OBJ_NEW_QSTR(MP_QSTR_server),       	(mp_obj_t)&pyb_server_type },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_wipy_globals, mp_module_wipy_globals_table);

const mp_obj_module_t mp_module_wipy = {
    .base = { &mp_type_module },
    .name = MP_QSTR_wipy,
    .globals = (mp_obj_dict_t*)&mp_module_wipy_globals_table,
};
