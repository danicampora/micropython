freeze("$(PORT_DIR)/modules")
include("$(MPY_DIR)/extmod/asyncio")
# Useful networking-related packages.
#require("bundle-networking")
# Require some micropython-lib modules.
require("onewire")
require("ds18x20")
require("dht")
require("neopixel")
require("upysh")
require("umqtt.robust")
require("umqtt.simple")
