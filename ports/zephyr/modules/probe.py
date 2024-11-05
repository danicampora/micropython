import machine
from machine import Pin, I2C
import struct
from micropython import const
from probe_protocol import *


PROBE_I2C_ADDR = const(0x7E)


class Probe:

    def __init__(self):     
        self.i2c = I2C(1, scl=Pin(36), sda=Pin(35), freq=50000)

    def _data_transfer(self, tx_buf, rx_len):
        self.i2c.writeto(PROBE_I2C_ADDR, tx_buf)
        return self.i2c.readfrom(PROBE_I2C_ADDR, rx_len)

    def _create_request_header(self, request_type, payload_len):
        return struct.pack(PROTOCOL_REQUEST_HEADER_FORMAT, request_type, payload_len)

    def _get_device_information(self):
        request = self._create_request_header(E_PROTOCOL_GET_DEVICE_INFO, 0)
        _dev_info_packed = self._data_transfer(request, struct.calcsize(PROTOCOL_RESPONSE_HEADER_FORMAT) + struct.calcsize(PROTOCOL_RESPONSE_DEV_INFO_FORMAT))
        _dev_info_packed = _dev_info_packed[struct.calcsize(PROTOCOL_RESPONSE_HEADER_FORMAT):]
        id_0, id_1, id_2, id_3, id_4, id_5, id_6, id_7, ver_0, ver_1, ver_2, reading_len, battery_level = struct.unpack(PROTOCOL_RESPONSE_DEV_INFO_FORMAT, _dev_info_packed)
        print('Device ID is {:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}'.format(id_7, id_6, id_5, id_4, id_3, id_2, id_1, id_0)) 
        print('FW version is {}.{}.{}'.format(ver_0, ver_1, ver_2))

    def _get_reading(self):
        request = self._create_request_header(E_PROTOCOL_GET_READING, 0)
        _dev_info_packed = self._data_transfer(request, struct.calcsize(PROTOCOL_RESPONSE_HEADER_FORMAT) + struct.calcsize(PROTOCOL_RESPONSE_READING_T1_FORMAT))
        _dev_info_packed = _dev_info_packed[struct.calcsize(PROTOCOL_RESPONSE_HEADER_FORMAT):]
        _temperature, _triggered = struct.unpack(PROTOCOL_RESPONSE_READING_T1_FORMAT, _dev_info_packed)
        print('T1 temperature value is: {:.2f} triggered: {}'.format(_temperature, True if _triggered else False))
