import iotcore
import probe
import time


prb = probe.Probe()
print('Probe Initialized...')
time.sleep(1)
iot = iotcore.IoTCore(prb)


while True:
    if iot.is_ready():
        try:
            print('Sending...')
            iot.send_ping(95.5, 3750)
            time.sleep(3)
            iot.send_device_information_reply(0, 95.5, 1, 3750)
            time.sleep(3)
            iot.send_port_utilization_reply(0, 4, 0b01)
            time.sleep(3)
            iot.send_port_occupied(1)
            time.sleep(3)
            iot.send_sensor_information_reply(0, 1)
            time.sleep(3)
            iot.send_sensor_data(1)
            time.sleep(3)
        except Exception:
            pass
        time.sleep_ms(5000)
    else:
        time.sleep_ms(100)
