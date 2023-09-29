#include "SdFatWavRK.h"


SYSTEM_THREAD(ENABLED);

SerialLogHandler logHandler;


const int SD_CHIP_SELECT = A5;
SdFat sd;
PrintFile curFile;
bool sdReady = false;

void setup() {
	// Wait for a USB serial connection for up to 15 seconds. Useful for the tester, less so for
	// normal firmware.
	waitFor(Serial.isConnected, 15000);

	sdReady = sd.begin(SD_CHIP_SELECT, SPI_FULL_SPEED);
	Log.info("sdReady=%d", sdReady);


	WavHeader<WavHeaderBase::STANDARD_SIZE> header;
	header.writeHeader(2, 22050, 16);

	Log.dump(header.getBuffer(), header.getBufferOffset());
}

void loop() {

}

