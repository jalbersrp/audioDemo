# Audio Demo

This demo shows audio processing in the Particle photon 2

The project takes 5 seconds of audio samples from the PDM microphone module and stores it in a SD card using .wav format with auto-incremental naming. Then it takes the first second of the saved sampling and performs FFT and MFCC computing to obtain the domimnant frequency and MFCC coefficients. Next it publishes the results on the particle cloud. The device is suscribed to an event from the cloud that can turn ON and OFF a relay module.

For triggering a sampling event, is possible to press the MODE button on the device or calling the function from cloud.

Compiled using VScode Particle Workbench 1.16.10 for deviceOS 5.5.0 and Photon 2.

### Cloud functions
Available from the console or cloud API:
* `audioSample` - Call this function to trigger a sampling and FFT compute analysis. Takes no arguments.
* `analyzeCoeff` - Call this function to trigger a MFCC coefficients compute on the last saved file. Takes no arguments.
* `relayControl` - Call this function to control the relay. Arguments: "OFF" , "ON". Whitout quotes.

### Cloud events
Responses from the device to the cloud:
* `dFrequency` - Dominant frequency value from the last sampling event.
* `coeff` - MFCC coefficient values from the last sampling event.

## Wiring
Photon 2 | SD module | PDM MIC | Relay module | Notes 
--- | --- | --- | --- | ---
GND|GND|GND|GND
VUSB|-|-|*|
3V3 | 3V3 | 3V | * |
S3  | CS |-|-
MO  |MOSI|-|-
MI|MISO|-|-
SCK| SCK|-|-
A1 | -|DAT|-
A0 | -|CLK|-
S4|-|-|S1|Input signal is inverted in the module (0V is ON)

*5V relays must be powered from VUSB pin,  3V relays must be powered from 3V3 pin.