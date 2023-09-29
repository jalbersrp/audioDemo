/******************************************************/
//       THIS IS A GENERATED FILE - DO NOT EDIT       //
/******************************************************/

#include "Particle.h"
#line 1 "/Users/alberto/Documents/Particle_Workbench/audioDemo/src/audioDemo.ino"
#include "Microphone_PDM.h"
#include <SdFatSequentialFileRK.h>
#include <SdFatWavRK.h>
#include <arduinoFFT.h>

//Logging
void setup();
void loop();
#line 7 "/Users/alberto/Documents/Particle_Workbench/audioDemo/src/audioDemo.ino"
SYSTEM_THREAD(ENABLED);													//Threading enabled
SerialLogHandler logHandler;											//Serial log handler object
//GPIO Pins
const int ledPin = D7;													//Pin for led indicator. D7 is the integrated blue led
const int relayPin = S4;												//Pin for relay
//SD and files
const int SD_CHIP_SELECT = S3;											//Chip select pin for SD card
SdFat sd;																//SD card object
PrintFile curFile;														//File object
SPISettings spiSettings(12 * MHZ, MSBFIRST, SPI_MODE0);					//SPI setting for SD card
SdFatSequentialFile sequentialFile(sd, SD_CHIP_SELECT, spiSettings);	//This creates unique, sequentially numbered files on the SD card.
SdFatWavWriter wavWriter(1, 16000, 16);									//This writes wav files to sdcard
//FFT
arduinoFFT FFT;															//FFT object
const uint16_t samples = 4096; 											//Number of samples to use for FFT. MUST ALWAYS be a power of 2
const double samplingFrequency = 16000;									//Sampling frequency (used by PDM module and FFT)
double vReal[samples]={0.0};											//Vector for samples (real part) to be analyzed
double vImag[samples]={0.0};											//Vector for samples (imaginary part) to be analyzed
double dominantFreq = 0.0;												//Stores the dominant frequency calculated
// Record
const unsigned long MAX_RECORDING_LENGTH_MS = 5 * 1000;					//Limits the recording length if MODE button is not pressed again
unsigned long recordingStart;											//Stores the start time for a recording
//State machine
enum State { STATE_WAITING, STATE_OPEN_FILE, STATE_RUNNING, STATE_FINISH, STATE_ANALYZE };		//State machine enums
State state = STATE_WAITING;											//Initial state of the state machine

//FORWARD DECLARATIONS
void buttonHandler(system_event_t event, int data);						//Event handler for MODE button (starts recording)
int raiseTrigger(String extra);											//Event handler for triggering the record from cloud
int relayControl(String state);
bool openNewFile();														//Creates a new file and adds the headers

//MAIN
void setup()
{
	System.on(button_click, buttonHandler);								//Register handler to handle clicking on the SETUP button
	Particle.function("audioSample", raiseTrigger);      				//Register a cloud function to start the target flashing
	Particle.function("relayControl", relayControl);      				//Register a cloud function to start the target flashing

	pinMode(ledPin, OUTPUT);											//Recording indicator as output
	digitalWrite(ledPin, LOW);											//Led off
	pinMode(relayPin, OUTPUT);											//Relay pin as output
	digitalWrite(relayPin, HIGH);										//Relay off (the module used has inverted logic)
	waitFor(Serial.isConnected, 10000);									//Optional for waiting to serial enabled to see logs

	sequentialFile
		.withDirName("audio")
		.withNamePattern("%06d.wav");									//This saves files with format xxxxxx.wav in the /audio directory (created if needed).

	int err = Microphone_PDM::instance()
		.withOutputSize(Microphone_PDM::OutputSize::SIGNED_16)
		.withRange(Microphone_PDM::Range::RANGE_2048)
		.withSampleRate(16000)
		.init();														//Initiales PDM microphone at 16 bits/sample and range scale of 2048
	if (err)
	{
		Log.error("PDM decoder init error: %d", err);
	}
	err = Microphone_PDM::instance().start();							//Starts sampling
	if (err)
	{
		Log.error("PDM decoder start error: %d", err);
	}
}

void loop()
{
	switch(state)
	{
	case STATE_WAITING:
		// Waiting for the user to press the MODE button. A press in the button will bump the state to OPEN_FILE
		break;

	case STATE_OPEN_FILE:
		//Opens a new file for starting a new recording and switch state to RUNNING
		if (openNewFile())
		{
			recordingStart = millis();
			digitalWrite(ledPin, HIGH);
			state = STATE_RUNNING;
		}
		else
		{
			Log.info("Failed to write new file to SD card.");
			state = STATE_WAITING;
		}
		break;

	case STATE_RUNNING:
		//Takes samples from microphone and stores in the new file
		Microphone_PDM::instance().noCopySamples([](void *pSamples, size_t numSamples) {
			curFile.write((uint8_t *)pSamples, Microphone_PDM::instance().getBufferSizeInBytes());
		});
		//If the recording reached the max time, switch state to FINISH.
		if (millis() - recordingStart >= MAX_RECORDING_LENGTH_MS)
		{
			state = STATE_FINISH;											
		}
		break;

	case STATE_FINISH:
		//Perform the final steps after the recording
	
		Log.info("Recording stopped.");
		digitalWrite(ledPin, LOW);										//Led off
		wavWriter.updateHeaderFromLength(&curFile);						//Updates the wav file header with the actual recording length
		curFile.close();												//Closes the file
		state = STATE_ANALYZE;											//Jump to analyze state
		break;

	case STATE_ANALYZE:
		//Performs FFT on the last file first samples 

		//Opens the last file and gets the filename
		if (sequentialFile.openFile(&curFile, false))
		{
			char name[14];
			if (curFile.getName(name, sizeof(name)))
			{
				Log.info("Analyzing %s" , name);
			}
			else
			{
				Log.info("Can't get file name.");
			}
		}
		else
		{
			Log.info("Failed to open the file for analysis.");
		}
        
		//Reads the first 44 bytes (header) of the WAV file, does nothing with that. Just to bring the read pointer to where the data starts
		char buffer[44];
		curFile.read(buffer,44);
        
		//.WAV data is stored on byte pairs (little endian)
		byte preBuff[samples * 2];										//Prebuffer to store the samples from the file. Size is 2 bytes times samples
		curFile.read(preBuff,samples * 2);								//Fills prebuffer with data from file
		curFile.close();												//Closes the file
		
		//Pass the data to the FFT samples buffer
		for(int i=0;i<samples;i++)
		{
			vReal[i]=(int16_t)(((preBuff[(i*2)+1]<<8) + preBuff[i*2])/32768);	//Takes a pair of bytes, corrects endianess and converts it to double before storing
			vImag[i]=0;													//Erases any last data stored here (last FFT process changed this data)
		}
		
		//Performs FFT
		FFT = arduinoFFT(vReal, vImag, samples, samplingFrequency);		//Create FFT object from real/imaginary data sampled, number of samples and the sampling frequency
		FFT.Windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);				//Weights the data
		FFT.Compute(FFT_FORWARD);										//Computes FFT
		FFT.ComplexToMagnitude();										//Computes magnitudes from complex numbers
		dominantFreq = FFT.MajorPeak();									//Gets the dominant frequency from the data

		Log.info("Major Peak: %f" , dominantFreq);
		
		//Publishes the result to the cloud
		if(!Particle.publish("dFrequency", String(dominantFreq)))
		{
			Log.info("Can't publish to cloud.");
		}					
		state = STATE_WAITING;											//Jumps to WAITING state
		break;
	}
}


//Functions

//Handler for the cloud function that triggers the recording
int raiseTrigger(String extra)
{
  //This function raises the flag to update the arduino board asset
  state = STATE_OPEN_FILE;
  return 0;
}

int relayControl(String state)
{
	if(state == "ON")
	{
		digitalWrite(relayPin, LOW);
	}
	if(state == "OFF")
	{
		digitalWrite(relayPin, HIGH);
	}
	return 0;
}

//Handler for the press of MODE button
void buttonHandler(system_event_t event, int data)
{
	//Toggles the recording state every time the button is pressed
	switch(state)
	{
		case STATE_WAITING:
			state = STATE_OPEN_FILE;
			break;

		case STATE_RUNNING:
			state = STATE_FINISH;
			break;
	}
}

//Opens a new file and adds the WAV header
bool openNewFile()
{
	bool success = false;
	if (sequentialFile.openFile(&curFile, true))
	{
		char name[14];
		curFile.getName(name, sizeof(name));
		if (wavWriter.startFile(&curFile))
		{
			Log.info("New file created %s", name);
			success = true;
		}
		else
		{
			Log.info("Error creating file header.");
		}
	}
	else
	{
		Log.info("File creation failed.");
	}
	return success;
}