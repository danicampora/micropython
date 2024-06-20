
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

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <nrf_modem_dect_phy.h>
#include <modem/nrf_modem_lib.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/logging/log.h>


#define CONFIG_CARRIER                              1677
#define CONFIG_NETWORK_ID                           91
#define CONFIG_MCS                                  1
#define CONFIG_TX_POWER                             11
#define CONFIG_TX_TRANSMISSIONS                     30
#define CONFIG_RX_PERIOD_S                          5

LOG_MODULE_REGISTER(dect);


// declare local private functions

static void init(const uint64_t *time, int16_t temp, enum nrf_modem_dect_phy_err err,
	  const struct nrf_modem_dect_phy_modem_cfg *cfg);

/* Callback after deinit operation. */
static void deinit(const uint64_t *time, enum nrf_modem_dect_phy_err err);

/* Operation complete notification. */
static void op_complete(const uint64_t *time, int16_t temperature,
		 enum nrf_modem_dect_phy_err err, uint32_t handle);

/* Callback after receive stop operation. */
static void rx_stop(const uint64_t *time, enum nrf_modem_dect_phy_err err, uint32_t handle);

/* Physical Control Channel reception notification. */
static void pcc(
	const uint64_t *time,
	const struct nrf_modem_dect_phy_rx_pcc_status *status,
	const union nrf_modem_dect_phy_hdr *hdr);

/* Physical Control Channel CRC error notification. */
static void pcc_crc_err(const uint64_t *time,
		 const struct nrf_modem_dect_phy_rx_pcc_crc_failure *crc_failure);

/* Physical Data Channel reception notification. */
static void pdc(const uint64_t *time,
		const struct nrf_modem_dect_phy_rx_pdc_status *status,
		const void *data, uint32_t len);

/* Physical Data Channel CRC error notification. */
static void pdc_crc_err(
	const uint64_t *time, const struct nrf_modem_dect_phy_rx_pdc_crc_failure *crc_failure);

/* RSSI measurement result notification. */
static void rssi(const uint64_t *time, const struct nrf_modem_dect_phy_rssi_meas *status);

/* Callback after link configuration operation. */
static void link_config(const uint64_t *time, enum nrf_modem_dect_phy_err err);

/* Callback after time query operation. */
static void time_get(const uint64_t *time, enum nrf_modem_dect_phy_err err);

/* Callback after capability get operation. */
static void capability_get(const uint64_t *time, enum nrf_modem_dect_phy_err err,
		    const struct nrf_modem_dect_phy_capability *capability);

/* Send operation. */
static int transmit(uint32_t handle, void *data, size_t data_len);

/* Receive operation. */
static int receive(uint32_t handle);

// declare private data

#define DATA_LEN_MAX 32

static bool exit;
static uint16_t device_id;

/* Header type 1, due to endianness the order is different than in the specification. */
struct phy_ctrl_field_common {
	uint32_t packet_length : 4;
	uint32_t packet_length_type : 1;
	uint32_t header_format : 3;
	uint32_t short_network_id : 8;
	uint32_t transmitter_id_hi : 8;
	uint32_t transmitter_id_lo : 8;
	uint32_t df_mcs : 3;
	uint32_t reserved : 1;
	uint32_t transmit_power : 4;
	uint32_t pad : 24;
};

/* Semaphore to synchronize modem calls. */
K_SEM_DEFINE(operation_sem, 0, 1);

/* Dect PHY callbacks. */
static struct nrf_modem_dect_phy_callbacks dect_phy_callbacks = {
	.init = init,
	.deinit = deinit,
	.op_complete = op_complete,
	.rx_stop = rx_stop,
	.pcc = pcc,
	.pcc_crc_err = pcc_crc_err,
	.pdc = pdc,
	.pdc_crc_err = pdc_crc_err,
	.rssi = rssi,
	.link_config = link_config,
	.time_get = time_get,
	.capability_get = capability_get,
};

/* Dect PHY init parameters. */
static struct nrf_modem_dect_phy_init_params dect_phy_init_params = {
	.harq_rx_expiry_time_us = 5000000,
	.harq_rx_process_count = 4,
};


// define module exported functions

static mp_obj_t dect_version(void) {
    return mp_obj_new_int(1);
}
static MP_DEFINE_CONST_FUN_OBJ_0(dect_version_obj, dect_version);

static mp_obj_t dect_init(size_t n_args, const mp_obj_t *args) {

	int err = nrf_modem_lib_init();
	if (err) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("DECT modem init failed, err %d"), err);
	}

	err = nrf_modem_dect_phy_callback_set(&dect_phy_callbacks);
	if (err) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("nrf_modem_dect_phy_callback_set failed, err %d"), err);
	}

	err = nrf_modem_dect_phy_init(&dect_phy_init_params);
	if (err) {
         mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("nrf_modem_dect_phy_init failed, err %d"), err);
	}

	k_sem_take(&operation_sem, K_FOREVER);
	if (exit) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("DECT semapahore take failed, err %d"), -EIO);
	}

	hwinfo_get_device_id((void *)&device_id, sizeof(device_id));

    mp_printf(&mp_plat_print, "Dect NR+ PHY initialized, device ID: %d", device_id);

	err = nrf_modem_dect_phy_capability_get();
	if (err) {
        mp_raise_msg_varg(&mp_type_OSError, MP_ERROR_TEXT("nrf_modem_dect_phy_capability_get failed, err %d"), err);
	}

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

/* Callback after init operation. */
static void init(const uint64_t *time, int16_t temp, enum nrf_modem_dect_phy_err err,
	  const struct nrf_modem_dect_phy_modem_cfg *cfg)
{
	if (err) {
		LOG_ERR("Init failed, err %d", err);
		exit = true;
		return;
	}

	k_sem_give(&operation_sem);
}

/* Callback after deinit operation. */
static void deinit(const uint64_t *time, enum nrf_modem_dect_phy_err err)
{
	if (err) {
		LOG_ERR("Deinit failed, err %d", err);
		return;
	}

	k_sem_give(&operation_sem);
}

/* Operation complete notification. */
static void op_complete(const uint64_t *time, int16_t temperature,
		 enum nrf_modem_dect_phy_err err, uint32_t handle)
{
	LOG_DBG("op_complete cb time %"PRIu64" status %d", *time, err);
	k_sem_give(&operation_sem);
}

/* Callback after receive stop operation. */
static void rx_stop(const uint64_t *time, enum nrf_modem_dect_phy_err err, uint32_t handle)
{
	LOG_DBG("rx_stop cb time %"PRIu64" status %d handle %d", *time, err, handle);
	k_sem_give(&operation_sem);
}

/* Physical Control Channel reception notification. */
static void pcc(
	const uint64_t *time,
	const struct nrf_modem_dect_phy_rx_pcc_status *status,
	const union nrf_modem_dect_phy_hdr *hdr)
{
	struct phy_ctrl_field_common *header = (struct phy_ctrl_field_common *)hdr->type_1;

	LOG_INF("Received header from device ID %d",
		header->transmitter_id_hi << 8 |  header->transmitter_id_lo);
}

/* Physical Control Channel CRC error notification. */
static void pcc_crc_err(const uint64_t *time,
		 const struct nrf_modem_dect_phy_rx_pcc_crc_failure *crc_failure)
{
	LOG_DBG("pcc_crc_err cb time %"PRIu64"", *time);
}

/* Physical Data Channel reception notification. */
static void pdc(const uint64_t *time,
		const struct nrf_modem_dect_phy_rx_pdc_status *status,
		const void *data, uint32_t len)
{
	/* Received RSSI value is in fixed precision format Q14.1 */
	LOG_INF("Received data (RSSI: %d.%d): %s",
		(status->rssi_2 / 2), (status->rssi_2 & 0b1) * 5, (char *)data);
}

/* Physical Data Channel CRC error notification. */
static void pdc_crc_err(
	const uint64_t *time, const struct nrf_modem_dect_phy_rx_pdc_crc_failure *crc_failure)
{
	LOG_DBG("pdc_crc_err cb time %"PRIu64"", *time);
}

/* RSSI measurement result notification. */
static void rssi(const uint64_t *time, const struct nrf_modem_dect_phy_rssi_meas *status)
{
	LOG_DBG("rssi cb time %"PRIu64" carrier %d", *time, status->carrier);
}

/* Callback after link configuration operation. */
static void link_config(const uint64_t *time, enum nrf_modem_dect_phy_err err)
{
	LOG_DBG("link_config cb time %"PRIu64" status %d", *time, err);
}

/* Callback after time query operation. */
static void time_get(const uint64_t *time, enum nrf_modem_dect_phy_err err)
{
	LOG_DBG("time_get cb time %"PRIu64" status %d", *time, err);
}

/* Callback after capability get operation. */
static void capability_get(const uint64_t *time, enum nrf_modem_dect_phy_err err,
		    const struct nrf_modem_dect_phy_capability *capability)
{
	LOG_DBG("capability_get cb time %"PRIu64" status %d", *time, err);
}

/* Send operation. */
static int transmit(uint32_t handle, void *data, size_t data_len)
{
	int err;

	struct phy_ctrl_field_common header = {
		.header_format = 0x0,
		.packet_length_type = 0x0,
		.packet_length = 0x01,
		.short_network_id = (CONFIG_NETWORK_ID & 0xff),
		.transmitter_id_hi = (device_id >> 8),
		.transmitter_id_lo = (device_id & 0xff),
		.transmit_power = CONFIG_TX_POWER,
		.reserved = 0,
		.df_mcs = CONFIG_MCS,
	};

	struct nrf_modem_dect_phy_tx_params tx_op_params = {
		.start_time = 0,
		.handle = handle,
		.network_id = CONFIG_NETWORK_ID,
		.phy_type = 0,
		.lbt_rssi_threshold_max = 0,
		.carrier = CONFIG_CARRIER,
		.lbt_period = NRF_MODEM_DECT_LBT_PERIOD_MAX,
		.phy_header = (union nrf_modem_dect_phy_hdr *)&header,
		.data = data,
		.data_size = data_len,
	};

	err = nrf_modem_dect_phy_tx(&tx_op_params);
	if (err != 0) {
		return err;
	}

	return 0;
}

/* Receive operation. */
static int receive(uint32_t handle)
{
	int err;

	struct nrf_modem_dect_phy_rx_params rx_op_params = {
		.start_time = 0,
		.handle = handle,
		.network_id = CONFIG_NETWORK_ID,
		.mode = NRF_MODEM_DECT_PHY_RX_MODE_CONTINUOUS,
		.rssi_interval = NRF_MODEM_DECT_PHY_RSSI_INTERVAL_OFF,
		.link_id = NRF_MODEM_DECT_PHY_LINK_UNSPECIFIED,
		.rssi_level = -60,
		.carrier = CONFIG_CARRIER,
		.duration = CONFIG_RX_PERIOD_S * MSEC_PER_SEC *
			    NRF_MODEM_DECT_MODEM_TIME_TICK_RATE_KHZ,
		.filter.short_network_id = CONFIG_NETWORK_ID & 0xff,
		.filter.is_short_network_id_used = 1,
		/* listen for everything (broadcast mode used) */
		.filter.receiver_identity = 0,
	};

	err = nrf_modem_dect_phy_rx(&rx_op_params);
	if (err != 0) {
		return err;
	}

	return 0;
}
