#ifndef _SDFATWAVRK_H
#define _SDFATWAVRK_H

#include "Particle.h"
#include "SdFat.h"

/**
 * @brief Class for manipulating wav file headers
 */
class WavHeaderBase {
public:
	WavHeaderBase(uint8_t *buffer, size_t bufferSize);
	virtual ~WavHeaderBase();

	/**
	 * @brief Writes the wav file header to the start of buffer and updates bufferOffset
	 *
	 * @param numChannels number of channels, typically 1 or 2
	 *
	 * @param sampleRate sampling rate per channel in samples per second. For example: 16000 is 16 kHz sampling rate.
	 *
	 * @param bitsPerSample the number of bits per sample per channel. Typically 8 or 16.
	 *
	 * @param dataSizeInBytes the size of the data (optional). If you don't pass this parameter or pass 0 you
	 * must call setDataSize later, as wav files must include the chunk length and the file length in the header.
	 * There is no facility for files bounded by the length of the file or stream.
	 */
	bool writeHeader(uint8_t numChannels, uint32_t sampleRate, uint8_t bitsPerSample, uint32_t dataSizeInBytes = 0);

	/**
	 * @brief Update the size of the data chunk (in bytes).
	 *
	 * @param dataSizeInBytes This is the size of the data (part of the file after getDataOffset()), which is
	 * typically 44 bytes less than the file size for files we create.
	 *
	 * This modifies the file chunk header and the data subchunk size.
	 */
	void setDataSize(uint32_t dataSizeInBytes);

	/**
	 * @brief Gets the offset of the data chunk
	 *
	 * When we create the file with writeHeader, this will be 44. When we read files, it's typically 44
	 * but could be larger probably.
	 */
	uint32_t getDataOffset() const;

	/**
	 * @brief Find a subchunk within the header
	 *
	 * @param id The subchunk id to find (typically 'fmt ' or 'data')
	 *
	 * @param chunkDataOffset Filled in with the file offset of this subchunk data (not including the header)
	 *
	 * @param chunkDataSize Filled in with the size of this subchunk data (not including the header)
	 *
	 * The entire header must be in buffer. This is usually 44 bytes, but files we didn't write could
	 * be larger with more subchunks.
	 */
	bool findChunk(uint32_t id, size_t &chunkDataOffset, uint32_t &chunkDataSize) const;

	void setUint16LE(size_t offset, uint16_t value);
	uint16_t getUint16LE(size_t offset) const;

	void setUint32LE(size_t offset, uint32_t value);
	uint32_t getUint32LE(size_t offset) const;

	static uint32_t fourCharStringToValue(const char *str);

	void setUint32BE(size_t offset, uint32_t value);
	uint32_t getUint32BE(size_t offset) const;

	size_t getBufferOffset() const { return bufferOffset; };

	uint8_t *getBuffer() { return buffer; };

	const uint8_t *getBuffer() const { return buffer; };

	size_t getBufferSize() const { return bufferSize; };

	/**
	 * @brief This is the size of the header we write using writeHeader.
	 *
	 * The buffer must be at least this large. For reading headers it will also often be 44
	 * bytes but it could be larger. Though some larger files (non-PCM files, for example)
	 * cannot be read. However, it could have extra subchunks, which the library will
	 * safely ignore.
	 */
	static const size_t STANDARD_SIZE = 44;

protected:
	uint8_t *buffer;
	size_t bufferSize;
	size_t bufferOffset = 0;
};

/**
 * @brief Templated class that allows the size of the buffer to be configured
 *
 * For writing wav headers, it must be at least 44 bytes.
 */
template <size_t BUFFER_SIZE>
class WavHeader : public WavHeaderBase {
public:
	explicit WavHeader() : WavHeaderBase(staticBuffer, BUFFER_SIZE) {};

private:
	uint8_t staticBuffer[BUFFER_SIZE]; //!< static buffer to write to
};


/**
 * @brief Class for using wav files on SD cards
 */
class SdFatWavWriter {
public:
	SdFatWavWriter();

	/**
	 * @brief Initialize object to
	 */
	SdFatWavWriter(uint8_t numChannels, uint32_t sampleRate, uint8_t bitsPerSample);
	virtual ~SdFatWavWriter();

	/**
	 * @brief Start writing to the file
	 *
	 * @param file The file to write to. It must be open and writeable. The contents will be deleted
	 * if it's not empty.
	 *
	 * The file header based on the parameters to the constructor will be written to the file.
	 * The current file position will be appropriate to write the sample data.
	 *
	 * Note: 8-bit samples are unsigned. 16-bit samples are signed little endian.
	 * Not really sure how 24 or 32-bit works in wav. Most are at most 16-bit.
	 */
	bool startFile(FatFile *file);

	/**
	 * @brief After writing all of the samples to the file, call this
	 *
	 * The wav file header includes the length of the data in two places, and it's required!
	 * Since we often do not know the exact file length when we start writing, we update it
	 * after all of the data has been written by calling this method.
	 */
	bool updateHeaderFromLength(FatFile *file);

	/**
	 * @brief Sets the number of channels. Usually 1 or 2.
	 *
	 * @param numChannels Number of channels
	 */
	SdFatWavWriter &withNumChannels(uint8_t numChannels) { this->numChannels = numChannels; return *this; };

	/**
	 * @brief Gets the number of channels. Usually 1 or 2.
	 */
	uint8_t getNumChannels() const { return numChannels; };

	/**
	 * @brief Sets the sample rate, for example 8000 is 8 kHz is sampling rate (per channel)
	 *
	 * @param sampleRate sample rate per channel in samples per second
	 */
	SdFatWavWriter &withSampleRate(uint32_t sampleRate) { this->sampleRate = sampleRate; return *this; };

	/**
	 * @brief Gets the sample rate, for example 8000 is 8 kHz is sampling rate (per channel)
	 */
	uint32_t getSampleRate() const { return sampleRate; };

	/**
	 * @brief Sets the bits per sample per channel. Typically 8 or 16
	 *
	 * @param bitsPerSample Number of bytes per sample per channel. Typically 8, 16, 24, or 32.
	 */
	SdFatWavWriter &withBitsPerSample(uint8_t bitsPerSample) { this->bitsPerSample = bitsPerSample; return *this; };

	/**
	 * @brief Gets the bits per sample per channel. Typically 8 or 16
	 */
	uint8_t getBitsPerSample() const { return bitsPerSample; };



protected:
	uint8_t numChannels;
	uint32_t sampleRate;
	uint8_t bitsPerSample;
	WavHeader<WavHeaderBase::STANDARD_SIZE> header;
	FatFile *file = 0;

};

#endif /* _SDFATWAVRK_H */
