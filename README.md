# pico-1wire-lib
Lightweight 1-Wire Protocol Library for Raspberry Pi Pico SDK

This library implements basic 1-Wire protocol functionality to enable
use of some common 1-Wire sensors more easily from programs written using [Raspberry Pi Pico SDK](https://www.raspberrypi.com/documentation/pico-sdk/).

This library borrows some ideas from following other 1-Wire libraries:
- [pico-onewire library](https://github.com/adamboardman/pico-onewire) by Adam Boardman
- [DS1830 mbed library](https://developer.mbed.org/components/DS1820/) by Erik Olieman

## Supported Devices

List of currently supported devices (devices not on the list may or may not work):

Device|Type|Notes
------|----|-----
DS18S20|Temperature sensor (9bit)|
DS1822|Temperature sensor (9-12bit)|
DS18B20|Temperature sensor (9-12bit)|
DS1825|Temperature sensor (9-12bit)|
DS28EA00|Temperature sensor (9-12bit)|Currently no support for IO support on this chip.
MAX31820|Temperature sensor (9-12bit)|
MAX31826|Temperature sensor (9-12bit)|Currently no support for EEPROM on this chip.


## Examples

See [pico-1wire-lib example](example/)

