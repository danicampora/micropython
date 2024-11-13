import time
import lorawan
import struct
import _thread
import version
from micropython import const


# device models

DEVICE_MODEL_S1 = const(1)
DEVICE_MODEL_G1_WIFI = const(2)
DEVICE_MODEL_G1_LTE = const(3)
DEVICE_MODEL_M1 = const(4)
DEVICE_MODEL_S1_TEMP = const(7)


# uplink message types

DEVICE_INFORMATION_REPLY_MSG_TYPE = const(10)
PORT_UTILIZATION_REPLY_MSG_TYPE = const(11)
SETTINGS_REPLY_MSG_TYPE = const(12)
SETTINGS_UPDATED_MSG_TYPE = const(13)
PING_MSG_TYPE = const(15)
UPDATE_SETTINGS_TO_IOTCORE_MSG_TYPE = const(16)
PORT_OCCUPIED_MSG_TYPE = const(17)
PORT_FREED_MSG_TYPE = const(18)
DEVICE_RESTARTED_MSG_TYPE = const(19)
FUOTA_STARTED_MSG_TYPE = const(20)
FUOTA_CANCELLED_MSG_TYPE = const(21)


# T1 downlink sensor messages

SENSOR_REQUEST_DEVICE_INFORMATION_MSG_TYPE = const(16)
SENSOR_UPDATE_SETTINGS_MSG_TYPE = const(17)
SENSOR_REQUEST_SETTINGS_MSG_TYPE = const(22)


# downlink message types

REQUEST_DEVICE_INFORMATION_MSG_TYPE = const(10)
REQUEST_PORT_UTILIZATION_MSG_TYPE = const(11)
REQUEST_SETTINGS_MSG_TYPE = const(12)
UPDATE_SETTINGS_MSG_TYPE = const(13)
RESTART_DEVICE_MSG_TYPE = const(14)
START_FUOTA_MSG_TYPE = const(15)
CANCEL_FUOTA_MSG_TYPE = const(16)


# T1 uplink sensor messages

SENSOR_DEVICE_INFORMATION_MSG_TYPE = const(5)
SENSOR_SETTINGS_UPDATED_MSG_TYPE = const(17)
SENSOR_DATA_MSG_TYPE = const(21)
SENSOR_REPORT_SETTINGS_MSG_TYPE = const(22)
SENSOR_UPDATE_SETTINGS_TO_IOTCORE_MSG_TYPE = const(23)
SENSOR_CYCLE_DATA_MSG_TYPE = const(24)


# T1 reading types

SENSOR_READING_TYPE_TIMER = const(0)
SENSOR_READING_TYPE_TRIGGER = const(1)


# time and size related constants

IOTCORE_RX_TIMEOUT_MS = const(250)
IOTCORE_RX_BUFFER_LEN = const(128)

IOTCORE_MSG_QUEUE_MIN_LEN = const(1)
IOTCORE_MSG_QUEUE_DEF_LEN = const(32)
IOTCORE_MSG_QUEUE_MAX_LEN = const(64)

IOTCORE_MSG_QUEUE_STREAM_MIN_COUNT = const(1)
IOTCORE_MSG_QUEUE_STREAM_DEF_COUNT = const(2)
IOTCORE_MSG_QUEUE_STREAM_MAX_COUNT = const(3)

IOTCORE_MAX_MESSAGES_PER_FRAME = const(2)

IOTCORE_MAX_PING_FAILURES = const(8)    # 24 hours at the default 3 hour send cycle


UPLINK_HEADER_FORMAT = '!HBBI'

# Module uplinks
MODULE_PING_UPLINK_FORMAT = '!fh'
MODULE_DEVICE_INFORMATION_UPLINK_FORMAT = '!HbbbfBh'
MODULE_PORT_UTILIZATION_UPLINK_FORMAT = '!BB'
MODULE_SETTINGS_UPLINK_FORMAT = '!HB'
MODULE_SETTINGS_UPDATED_UPLINK_FORMAT = '!B'
MODULE_RESTARTED_UPLINK_FORMAT = '!B'
MODULE_PORT_OCCUPIED_UPLINK_FORMAT = '!BBBBBBBBB'
MODULE_PORT_FREED_UPLINK_FORMAT = '!BBBBBBBBB'

# Sensor uplinks
SENSOR_DEVICE_INFO_FORMAT = '!BBBBBBBBBBBB'
SENSOR_SETTINGS_UPDATED_FORMAT = '!B'
SENSOR_DATA_FORMAT = '!fB'
SENSOR_CYCLE_DATA_FORMAT = '!fffHHI'
SENSOR_REPORT_SETTINGS_FORMAT = '!HhhBBBBHBB'


DOWNLINK_HEADER_FORMAT = '!HBB'

# Module downlinks
MODULE_UPDATE_SETTINGS_DOWNLINK_FORMAT = '!HB'

# Sensor downlinks
SENSOR_UPDATE_SETTINGS_DOWNLINK_FORMAT = '!HhhBBBBHBB'


IOTCORE_LORAWAN_TX_CONFIRMED = True

# IOTCORE_LORA_DR = DR_5

IOTCORE_MODULE_PORT = const(0)

IOTCORE_LORAWAN_PORT_NUM = const(2)

IOTCORE_MAX_SENSORS = const(4)


class IoTCore:

    def __init__(self, probe):
        self.probe = probe
        self.lock = _thread.allocate_lock()
        lorawan.init()
        _thread.start_new_thread(self._run, ())

    def _run(self):
        while True:
            if not lorawan.has_joined():
                try:
                    lorawan.join()
                except Exception:
                    pass
            else:
                _rx_packet = lorawan.recv()
                if _rx_packet:
                    self._process_rx_data(_rx_packet)

    def _process_rx_data(self, rx_data):
        _header = rx_data[:struct.calcsize(DOWNLINK_HEADER_FORMAT)]
        _payload = rx_data[struct.calcsize(DOWNLINK_HEADER_FORMAT):]
        _message_id, _port_number, _message_type = struct.unpack(_header, DOWNLINK_HEADER_FORMAT)
        if _port_number == 0:
            if _message_type == REQUEST_DEVICE_INFORMATION_MSG_TYPE:
                 self.send_device_information_reply(_message_id, 97.77, 3670)
            elif _message_type == REQUEST_PORT_UTILIZATION_MSG_TYPE:
                 self.send_port_utilization_reply(_message_id, IOTCORE_MAX_SENSORS, 0b01)
            elif _message_type == REQUEST_SETTINGS_MSG_TYPE:
                 pass
            elif _message_type == UPDATE_SETTINGS_MSG_TYPE:
                 pass
            elif _message_type == RESTART_DEVICE_MSG_TYPE:
                 pass
        else:
            if _message_type == SENSOR_REQUEST_DEVICE_INFORMATION_MSG_TYPE:
                 pass
            elif _message_type == SENSOR_UPDATE_SETTINGS_MSG_TYPE:
                 pass
            elif _message_type == SENSOR_REQUEST_SETTINGS_MSG_TYPE:
                 pass

    def _prepare_header(self, mssg_id, port_number, msg_type):
        return struct.pack(UPLINK_HEADER_FORMAT, mssg_id, port_number, msg_type, time.time())

    def _send_packet(self, packet):
        with self.lock:
            lorawan.send(packet, IOTCORE_LORAWAN_PORT_NUM, IOTCORE_LORAWAN_TX_CONFIRMED)

    def is_ready(self):
        return lorawan.has_joined()

    def send_ping(self, charge, voltage):
        _packet = struct.pack('!B', 1)
        _packet += self._prepare_header(0, IOTCORE_MODULE_PORT, PING_MSG_TYPE)
        _packet += struct.pack(MODULE_PING_UPLINK_FORMAT, charge, voltage)
        self._send_packet(_packet)

    def send_sensor_data(self, temperature, trigger_type):
        _packet = struct.pack('!B', 1)
        _packet += self._prepare_header(0, IOTCORE_LORAWAN_PORT_NUM, IOTCORE_MODULE_PORT, SENSOR_DATA_MSG_TYPE)
        _packet += struct.pack(SENSOR_DATA_FORMAT, temperature, trigger_type)
        self._send_packet(_packet)

    def send_device_information_reply(self, msg_id, charge, connected_sensors, voltage):
        _packet = struct.pack('!B', 1)
        _packet += self._prepare_header(msg_id, IOTCORE_MODULE_PORT, DEVICE_INFORMATION_REPLY_MSG_TYPE)
        _packet += struct.pack(MODULE_DEVICE_INFORMATION_UPLINK_FORMAT, DEVICE_MODEL_M1,
                               int(version.FW_VERSION_NUM[1]), int(version.FW_VERSION_NUM[2]), int(version.FW_VERSION_NUM[3]),
                               charge, connected_sensors, voltage)
        self._send_packet(_packet)

    def send_port_utilization_reply(self, msg_id, bitmap_len, ports_used):
        _packet = struct.pack('!B', 1)
        _packet += self._prepare_header(msg_id, IOTCORE_MODULE_PORT, PORT_UTILIZATION_REPLY_MSG_TYPE)
        _packet += struct.pack(MODULE_PORT_UTILIZATION_UPLINK_FORMAT, bitmap_len, ports_used)
        self._send_packet(_packet)

    def send_port_occupied(self, port_number, sensor_id):
        _packet = struct.pack('!B', 1)
        _packet += self._prepare_header(0, IOTCORE_MODULE_PORT, PORT_OCCUPIED_MSG_TYPE)
        _packet += struct.pack(MODULE_PORT_OCCUPIED_UPLINK_FORMAT, port_number, sensor_id)
        self._send_packet(_packet)

    def send_port_freed(self, port_number):
        _packet = struct.pack('!B', 1)
        _packet += self._prepare_header(0, IOTCORE_MODULE_PORT, PORT_FREED_MSG_TYPE)
        _packet += struct.pack(MODULE_PORT_FREED_UPLINK_FORMAT, port_number)
        self._send_packet(_packet)

    def send_settings_reply(self, msg_id, cycle, operating_mode):
        _packet = struct.pack('!B', 1)
        _packet += self._prepare_header(msg_id, IOTCORE_MODULE_PORT, SETTINGS_REPLY_MSG_TYPE)
        _packet += struct.pack(MODULE_SETTINGS_UPLINK_FORMAT, cycle, operating_mode)
        self._send_packet(_packet)

    def send_settings_updated(self, msg_id, updated):
        _packet = struct.pack('!B', 1)
        _packet += self._prepare_header(msg_id, IOTCORE_MODULE_PORT, SETTINGS_UPDATED_MSG_TYPE)
        _packet += struct.pack(MODULE_SETTINGS_UPDATED_UPLINK_FORMAT, updated)
        self._send_packet(_packet)

    def send_device_restarted(self, msg_id, restarted):
        _packet = struct.pack('!B', 1)
        _packet += self._prepare_header(msg_id, IOTCORE_MODULE_PORT, DEVICE_RESTARTED_MSG_TYPE)
        _packet += struct.pack(MODULE_RESTARTED_UPLINK_FORMAT, restarted)
        self._send_packet(_packet)

    def send_sensor_information_reply(self, msg_id, port):
        self.probe.get_device_information(port)
        pass

    def send_sensor_settings_reply(self, msg_id, port, update_iot_core):
        pass

    def send_sensor_settings_updated(self, port, updated):
        pass
