import iotcore
import probe
import time


prb = probe.Probe()
iot = iotcore.IoTCore(prb)


while True:
    if iot.is_ready():
        try:
            print('Sending...')
            if iot.send_ping(95.5, 3750):
                time.sleep(2)
                iot.send_device_information_reply(0, 95.5, 1, 3750)
                time.sleep(2)
                iot.send_port_utilization_reply(0, 4, 0b01)
        except Exception:
            pass
        time.sleep_ms(5000)
    else:
        time.sleep_ms(100)
