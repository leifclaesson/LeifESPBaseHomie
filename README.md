# LeifESPBaseHomie

Helper functions to initialize LeifHomieLib together with LeifESPBase to form a homie-capable ESP8266 or ESP32-based device

It is designed for use _by programmers_.

For example, SSID, Key, host name, MQTT credentials are hardcoded and not settable from any user interface.
These and other things were *conscious design decisions* that reduce code complexity and resource requirements.

It's designed to help in the construction of _purpose-built specialty devices_. It is _not_ designed to build user-configurable devices.

Enter your WiFi SSID/key to `LeifESPBase\environment_setup.h`
Enter your MQTT credentials to `LeifESPBaseHomie\environment_setup.h`


## dependencies

[LeifESPBase](https://github.com/liket/LeifESPBase)
[LeifHomieLib](https://github.com/liket/LeifHomieLib)
[PangolinMQTT](https://github.com/leifclaesson/PangolinMQTT)

## dependencies (ESP32)

ArduinoOTA
ESP32
ESPmDNS
FS
Update
WebServer
WiFi

## dependencies (ESP8266)

ArduinoOTA
ESP8266mDNS
ESP8266-ping
ESP8266WebServer
ESP8266WiFi
ESPAsyncTCP

