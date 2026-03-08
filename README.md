# ESP32 TFT MQTT weather display

## Description

ESP32 based module for displaying weather data from my weather station posted on my MQTT broker.
Also supports OTA updates.

---

## Hardware

- MH-ET Live ESP32 Dev-Kit
- 1,3" IPS ST7789 240*240 display

---

## Needed changes for usage

The following lines need to be changed to your environment.

**WLAN SSID and PSK**
```cpp
#define WLAN_SSID "<SSID>"
#define WLAN_PASS "<PSK>"
```

**MQTT server IP**
```cpp
#define MQTT_SERVER      "xxx.xxx.xxx.xxx"
```

**Hostname under which it can be found in ArduinoOTA**
```cpp
ArduinoOTA.setHostname("<HOSTNAME>");
```

**Password for ArduinoOTA**
```cpp
ArduinoOTA.setPassword("<PASSWORD>");
```

---

## JSON format

The data displayed needs the following JSON format:
```json
{"source":"weather","location":"location","sensor":"SHT41_Temperature","value":  4.16}
{"source":"weather","location":"location","sensor":"SHT41_Humidity","value": 64.86}
```

---

# License

The ESP32 MQTT weather display project by Dominik Künne is licensed under CC BY-NC 4.0. To view a copy of this license, visit  https://creativecommons.org/licenses/by-nc/4.0

Please see the LICENSE file for details.