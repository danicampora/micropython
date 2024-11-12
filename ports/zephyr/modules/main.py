import iotcore
import time

iot = iotcore.IoTCore()

while True:
    if iot.is_ready():
        try:
            iot.send_ping()
        except Exception:
            pass
        time.sleep(5)
    else:
        time.sleep(0.1)
