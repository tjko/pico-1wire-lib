# pico-1wire-lib Example Program

Small example program that demonstrates enumerating
temperature sensors and reading temperatures.


## Compiling Example Program

First, make sure Pico-SDK is installed and setup correctly (```$PICO_SDK_PATH``` environment variable set).

Build steps (assuming using a Pico W):
```
$ cd example
$ mkdir build
$ cd build
$ cmake -DPICO_BOARD=pico_w ..
$ make
```


## Example Output

Example console output from the example program running on a Pico that
has three DS18B20 sensors connected to it (with 4.7K pullup between data pin and 3.3V).

```

BOOT
[     0.252490][     252] Check for device(s) in the bus...
[     0.256145][       3] 1 or more devices detected.
[     0.259940][       3] Checking for devices using phantom power...
[     0.268210][       8] No devices using phantom power found.
[     0.271739][       3] Send Read ROM Command...
[     0.282013][      10] Read ROM Failed (multiple devices in the bus?)
[     0.285675][       3] start loop
[     0.288972][       3] Find devices in the bus...
[     0.337435][      48] 3 device(s) found.
[     0.338713][       1] Device 01: 2882126b00000061
[     0.343454][       4] Device 02: 28821e6a000000cf
[     0.348227][       4] Device 03: 2871d86a0000005a
[     0.352988][       4] Convert temperature: all devices
[     0.360252][       7] Wait for temperature measurement to complete (750ms)...
[     1.115440][     755] Wait done.
[     1.127281][      11] Device 2882126B00000061: temp:  24.5000C
[     1.141523][      14] Device 28821E6A000000CF: temp:  24.2500C
[     1.155743][      14] Device 2871D86A0000005A: temp:  37.6250C
[     1.158877][       3] sleep...
[    11.162002][   10003] Find devices in the bus...
[    11.207650][      45] 3 device(s) found.
[    11.208909][       1] Device 01: 2882126b00000061
[    11.213670][       4] Device 02: 28821e6a000000cf
[    11.218444][       4] Device 03: 2871d86a0000005a
[    11.223200][       4] Convert temperature: all devices
[    11.230463][       7] Wait for temperature measurement to complete (750ms)...
[    11.985614][     755] Wait done.
[    11.997205][      11] Device 2882126B00000061: temp:  24.4375C
[    12.011424][      14] Device 28821E6A000000CF: temp:  24.2500C
[    12.025637][      14] Device 2871D86A0000005A: temp:  37.6875C
[    12.028770][       3] sleep...
```