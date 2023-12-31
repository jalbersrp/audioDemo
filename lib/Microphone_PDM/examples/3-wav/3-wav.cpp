#include "Microphone_PDM.h"
#include "MicWavWriter.h"

// This example prints a wav file to the USB serial debugging port in hex
// particle serial monitor | tee t1.hex
// (delete the first few non-hex lines of t1.txt)
// xxd -r -p t1.hex t1.wav

SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(SEMI_AUTOMATIC);

SerialLogHandler logHandler;

// How long to record in milliseconds
const unsigned long RECORDING_LENGTH_MS = 2000;


// Forward declarations
void buttonHandler(system_event_t event, int data);

bool startRecording = false;

void setup() {
	Particle.connect();

	// Register handler to handle clicking on the SETUP button
	System.on(button_click, buttonHandler);

	// Blue D7 LED indicates recording is on
	pinMode(D7, OUTPUT);
	digitalWrite(D7, LOW);

	// Optional, just for testing so I can see the logs below
	// waitFor(Serial.isConnected, 10000);

	// output size is typically Microphone_PDM::OutputSize::UNSIGNED_8 or icrophone_PDM::OutputSize::SIGNED_16
	// and must match what the server is expecting (set on the command line)

	// Range must match the microphone. The Adafruit PDM microphone is 12-bit, so Microphone_PDM::Range::RANGE_2048.

	// The sample rate default to 16000. The other valid value is 8000.

	int err = Microphone_PDM::instance()
		.withOutputSize(Microphone_PDM::OutputSize::UNSIGNED_8)
		.withRange(Microphone_PDM::Range::RANGE_2048)
		.withSampleRate(16000)
		.init();

	if (err) {
		Log.error("PDM decoder init err=%d", err);
	}

	err = Microphone_PDM::instance().start();
	if (err) {
		Log.error("PDM decoder start err=%d", err);
	}

}

void loop() {
    Microphone_PDM::instance().loop();

    if (startRecording) {
        startRecording = false;
	    digitalWrite(D7, HIGH);

        Microphone_PDM_BufferSampling_wav *samplingBuffer = new Microphone_PDM_BufferSampling_wav();

        samplingBuffer->withCompletionCallback([](uint8_t *buf, size_t bufSize) {
    	    digitalWrite(D7, LOW);

			for(size_t offset = 0; offset < bufSize; offset += 64) {
				size_t count = bufSize - offset;
				if (count > 64) {
					count = 64;
				}
				Log.dump(&buf[offset], count);
			}
        });
        samplingBuffer->withDurationMs(RECORDING_LENGTH_MS);


        Microphone_PDM::instance().bufferSamplingStart(samplingBuffer);
    }

}


// button handler for the SETUP button, used to toggle recording on and off
void buttonHandler(system_event_t event, int data) {
	startRecording = true;
}






