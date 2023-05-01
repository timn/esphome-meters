Holley DTZ541-ZEBA Power Meter
==============================

The Holley DTZ541-ZEBA is a common power meter in Germany. It provides data
via a standardized optical infrared connection. You can use the typical devices
in the market tha texpose the received data via UART.

The specific variant and version matters. I found that I could not use the
standard SML module in ESPHome to read the meter data, since it would only
send truncated messages (missing checksum and end marker). So I wrote some
custom to read out the meter. It is based on the standard documentation, some
info from libsml, and Tasmota. It uses a similar pattern matching approach to
Tasmota.

I found this not to work with the DTZ541-ZDBA (two tarriff counter).

<p align="center">
  <img src="power_meter.jpg?raw=true" alt="Power meter with IR comm head and ESP32 DevKitC"/>
</p>

