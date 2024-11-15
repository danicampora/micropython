import iotcore
import probe
import time


prb = probe.Probe()
iot = iotcore.IoTCore(prb)


while True:
    if iot.is_ready():
        try:
            iot.send_ping()
        except Exception:
            pass
        time.sleep(5)
    else:
        time.sleep(0.1)
