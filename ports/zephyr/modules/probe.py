import binascii
from machine import Pin, I2C
import struct
from micropython import const
from probe_protocol import *
import time


PROBE_I2C_ADDR = const(0x7E)


class Settings:

    def __init__(self, updated_at, cycle, trigger_from, trigger_to, operating_from, operating_to, trigger_delay, mode, trigger_window, sampling, min_max, offline_updated):
        self.updated_at = updated_at
        self.cycle = cycle
        self.trigger_from = trigger_from
        self.trigger_to = trigger_to
        self.operating_from = operating_from
        self.operating_to = operating_to
        self.trigger_delay = trigger_delay
        self.mode = mode
        self.trigger_window = trigger_window
        self.sampling = sampling
        self.min_max = min_max
        self.offline_updated = offline_updated
        self.dummy = 0


class Information:

    def __init__(self, id, version, reading_len, battery_level):
        self.id = id
        self.version = version
        self.reading_len = reading_len
        self.battery_level = battery_level


class Probe:

    def __init__(self):     
        self.i2c = I2C("i2c0")

    def _data_transfer(self, tx_buf, rx_len):
        self.i2c.writeto(PROBE_I2C_ADDR, tx_buf)
        return self.i2c.readfrom(PROBE_I2C_ADDR, rx_len)

    def _create_request_header(self, request_type, payload_len):
        return struct.pack(PROTOCOL_REQUEST_HEADER_FORMAT, request_type, payload_len)

    def _perform_request(self, request_type, request_payload, response_payload_format, port):
        _request_payload_len = 0
        if request_payload is not None:
            _request_payload_len = len(request_payload)
        request = self._create_request_header(request_type, _request_payload_len)
        _response_packed = self._data_transfer(request, struct.calcsize(PROTOCOL_RESPONSE_HEADER_FORMAT) + (struct.calcsize(response_payload_format) if response_payload_format is not None else 0))
        _response_header = _response_packed[:struct.calcsize(PROTOCOL_RESPONSE_HEADER_FORMAT)]
        _result, _response_payload_len = struct.unpack(PROTOCOL_RESPONSE_HEADER_FORMAT, _response_header)
        _response_packed = _response_packed[struct.calcsize(PROTOCOL_RESPONSE_HEADER_FORMAT):]
        if _result == 0:
            return struct.unpack(response_payload_format, _response_packed)
        else:
            return None

    def set_device_id(self, id, port):
        id = binascii.unhexlify(id)
        _unique_id_packed = struct.pack(PROTOCOL_REQUEST_SET_ID_FORMAT,
                                        int(id[0]),
                                        int(id[1]),
                                        int(id[2]),
                                        int(id[3]),
                                        int(id[4]),
                                        int(id[5]),
                                        int(id[6]),
                                        int(id[7]))
        return self._perform_request(E_PROTOCOL_SET_UNIQUE_ID, _unique_id_packed, None, port)

    def get_status(self, port):
        return self._perform_request(E_PROTOCOL_GET_STATUS, None, PROTOCOL_RESPONSE_STATUS_FORMAT, port)

    def get_device_information(self, port):
        request = self._create_request_header(E_PROTOCOL_GET_DEVICE_INFO, 0)
        _dev_info_packed = self._data_transfer(request, struct.calcsize(PROTOCOL_RESPONSE_HEADER_FORMAT) + struct.calcsize(PROTOCOL_RESPONSE_DEV_INFO_FORMAT))
        _dev_info_packed = _dev_info_packed[struct.calcsize(PROTOCOL_RESPONSE_HEADER_FORMAT):]
        id_0, id_1, id_2, id_3, id_4, id_5, id_6, id_7, ver_0, ver_1, ver_2, reading_len, battery_level = struct.unpack(PROTOCOL_RESPONSE_DEV_INFO_FORMAT, _dev_info_packed)
        return Information(struct.pack('!BBBBBBBB', id_7, id_6, id_5, id_4, id_3, id_2, id_1, id_0, ver_0), struct.pack('!BBB', ver_0, ver_1, ver_2), reading_len, battery_level)
        #return (id_7, id_6, id_5, id_4, id_3, id_2, id_1, id_0, ver_0, ver_1, ver_2, reading_len, battery_level)
        #print('Device ID is {:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}'.format(id_7, id_6, id_5, id_4, id_3, id_2, id_1, id_0))
        #print('FW version is {}.{}.{}'.format(ver_0, ver_1, ver_2))

    def get_reading(self, port):
        request = self._create_request_header(E_PROTOCOL_GET_READING, 0)
        _reading_packed = self._data_transfer(request, struct.calcsize(PROTOCOL_RESPONSE_HEADER_FORMAT) + struct.calcsize(PROTOCOL_RESPONSE_READING_T1_FORMAT))
        _reading_packed = _reading_packed[struct.calcsize(PROTOCOL_RESPONSE_HEADER_FORMAT):]
        _temperature, _triggered = struct.unpack(PROTOCOL_RESPONSE_READING_T1_FORMAT, _reading_packed)
        return (_temperature, _triggered)
        #print('T1 temperature value is: {:.2f} triggered: {}'.format(_temperature, True if _triggered else False))

    def get_min_reading(self, port):
        return self._perform_request(E_PROTOCOL_GET_MIN_READING, None, PROTOCOL_RESPONSE_MIN_MAX_READING_T1_FORMAT, port)

    def get_max_reading(self, port):
        return self._perform_request(E_PROTOCOL_GET_MAX_READING, None, PROTOCOL_RESPONSE_MIN_MAX_READING_T1_FORMAT, port)

    def get_device_settings(self, port):
        try:
            updated_at, cycle, trigger_from, trigger_to, operating_from, operating_to, trigger_delay, mode, trigger_window, sampling, min_max, offline_updated, dummy = self._perform_request(E_PROTOCOL_GET_SETTINGS, None, PROTOCOL_RESPONSE_SETTINGS_FORMAT, port)
            return Settings(updated_at, cycle, trigger_from, trigger_to, operating_from, operating_to, trigger_delay, mode, trigger_window, sampling, min_max, offline_updated)
        except Exception:
            return None

    def set_device_settings(self, settings, port):
        settings.updated_at = time.time()
        settings_packed = struct.pack(PROTOCOL_REQUEST_SETTINGS_FORMAT,
                                      settings.updated_at,
                                      settings.cycle,
                                      settings.trigger_from,
                                      settings.trigger_to,
                                      settings.operating_from,
                                      settings.operating_to,
                                      settings.trigger_delay,
                                      settings.mode,
                                      settings.trigger_window,
                                      settings.sampling,
                                      settings.min_max,
                                      settings.dummy)
        return self._perform_request(E_PROTOCOL_UPDATE_SETTINGS, settings_packed, None, port)

    def ping(self, port):
        return self.get_status(port)

    def sleep(self, port):
        return self._perform_request(E_PROTOCOL_GO_TO_SLEEP, None, None, port)

    def wake(self, port):
        pass

    def scan_triggers(self, port):
        pass
