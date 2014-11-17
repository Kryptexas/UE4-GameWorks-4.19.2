// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AudioDecompress.h: Unreal audio vorbis decompression interface object.
=============================================================================*/

#pragma once

/**
 * Whether to use OggVorbis audio format.
 **/
#ifndef WITH_OGGVORBIS
	#if PLATFORM_DESKTOP
		#define WITH_OGGVORBIS 1
	#else
		#define WITH_OGGVORBIS 0
	#endif
#endif

// 186ms of 44.1KHz data
// 372ms of 22KHz data
#define MONO_PCM_BUFFER_SAMPLES		8192
#define MONO_PCM_BUFFER_SIZE		( MONO_PCM_BUFFER_SAMPLES * sizeof( int16 ) )

/**
 * Loads voribs dlls
*/
ENGINE_API void LoadVorbisLibraries();

/**
 * Interface class to decompress various types of audio data
 */
class ICompressedAudioInfo
{
public:
	/**
	* Virtual destructor.
	*/
	virtual ~ICompressedAudioInfo() { }

	/**
	* Reads the header information of a compressed format
	*
	* @param	InSrcBufferData		Source compressed data
	* @param	InSrcBufferDataSize	Size of compressed data
	* @param	QualityInfo			Quality Info (to be filled out)
	*/
	ENGINE_API virtual bool ReadCompressedInfo(const uint8* InSrcBufferData, uint32 InSrcBufferDataSize, struct FSoundQualityInfo* QualityInfo) = 0;

	/**
	* Decompresses data to raw PCM data.
	*
	* @param	Destination	where to place the decompressed sound
	* @param	bLooping	whether to loop the sound by seeking to the start, or pad the buffer with zeroes
	* @param	BufferSize	number of bytes of PCM data to create
	*
	* @return	bool		true if the end of the data was reached (for both single shot and looping sounds)
	*/
	ENGINE_API virtual bool ReadCompressedData(uint8* Destination, bool bLooping, uint32 BufferSize) = 0;

	/**
	 * Seeks to time (Some formats might not be seekable)
	 */
	virtual void SeekToTime(const float SeekTime) = 0;

	/**
	* Decompress an entire data file to a TArray
	*/
	ENGINE_API virtual void ExpandFile(uint8* DstBuffer, struct FSoundQualityInfo* QualityInfo) = 0;

	/**
	* Sets decode to half-rate
	*
	* @param	HalfRate	Whether Half rate is enabled
	*/
	ENGINE_API virtual void EnableHalfRate(bool HalfRate) = 0;

	/**
	 * Gets the size of the source buffer originally passed to the info class (bytes)
	 */
	virtual uint32 GetSourceBufferSize() const = 0;
};

#if WITH_OGGVORBIS
/** 
 * Helper class to parse ogg vorbis data
 */
class FVorbisAudioInfo : public ICompressedAudioInfo
{
public:
	ENGINE_API FVorbisAudioInfo( void );
	ENGINE_API virtual ~FVorbisAudioInfo( void );

	/** Emulate read from memory functionality */
	size_t			Read( void *ptr, uint32 size );
	int				Seek( uint32 offset, int whence );
	int				Close( void );
	long			Tell( void );

	/** 
	 * Reads the header information of an ogg vorbis file
	 * 
	 * @param	Resource		Info about vorbis data
	 */
	ENGINE_API virtual bool ReadCompressedInfo( const uint8* InSrcBufferData, uint32 InSrcBufferDataSize, struct FSoundQualityInfo* QualityInfo );

	/** 
	 * Decompresses ogg data to raw PCM data. 
	 * 
	 * @param	Destination	where to place the decompressed sound
	 * @param	bLooping	whether to loop the sound by seeking to the start, or pad the buffer with zeroes
	 * @param	BufferSize	number of bytes of PCM data to create
	 *
	 * @return	bool		true if the end of the data was reached (for both single shot and looping sounds)
	 */
	ENGINE_API virtual bool ReadCompressedData( uint8* Destination, bool bLooping, uint32 BufferSize );

	ENGINE_API virtual void SeekToTime( const float SeekTime );

	/** 
	 * Decompress an entire ogg data file to a TArray
	 */
	ENGINE_API virtual void ExpandFile( uint8* DstBuffer, struct FSoundQualityInfo* QualityInfo );

	/** 
	 * Sets ogg to decode to half-rate
	 * 
	 * @param	Resource		Info about vorbis data
	 */
	ENGINE_API virtual void EnableHalfRate( bool HalfRate );

	virtual uint32 GetSourceBufferSize() const { return SrcBufferDataSize;}

	struct FVorbisFileWrapper* VFWrapper;
	const uint8*		SrcBufferData;
	uint32			SrcBufferDataSize;
	uint32			BufferOffset;
};
#endif

/**
* Helper class to parse opus data
*/
class FOpusAudioInfo : public ICompressedAudioInfo
{
public:
	ENGINE_API FOpusAudioInfo(void);
	ENGINE_API virtual ~FOpusAudioInfo(void);

	/** Emulate read from memory functionality */
	size_t			Read(void *ptr, uint32 size);

	// ICompressedAudioInfo Interface
	ENGINE_API virtual bool ReadCompressedInfo(const uint8* InSrcBufferData, uint32 InSrcBufferDataSize, struct FSoundQualityInfo* QualityInfo);
	ENGINE_API virtual bool ReadCompressedData(uint8* Destination, bool bLooping, uint32 BufferSize);
	ENGINE_API virtual void SeekToTime(const float SeekTime) {};
	ENGINE_API virtual void ExpandFile(uint8* DstBuffer, struct FSoundQualityInfo* QualityInfo);
	ENGINE_API virtual void EnableHalfRate(bool HalfRate) {};
	virtual uint32 GetSourceBufferSize() const { return SrcBufferDataSize;}
	// End of ICompressedAudioInfo Interface

	struct FOpusDecoderWrapper* OpusDecoderWrapper;
	const uint8*	SrcBufferData;
	uint32			SrcBufferDataSize;
	uint32			SrcBufferOffset;
	uint32			AudioDataOffset;

	uint32			TrueSampleCount;
	uint32			CurrentSampleCount;
	uint8			NumChannels;

	TArray<uint8>	LastDecodedPCM;
	uint32			LastPCMByteSize;
	uint32			LastPCMOffset;
};

/**
 * Asynchronous audio decompression
 */
class FAsyncAudioDecompressWorker : public FNonAbandonableTask
{
protected:
	class USoundWave*		Wave;

public:
	/**
	 * Async decompression of audio data
	 *
	 * @param	InWave		Wave data to decompress
	 */
	FAsyncAudioDecompressWorker(USoundWave* InWave)
		: Wave(InWave)
	{
	}

	/**
	 * Performs the async audio decompression
	 */
	ENGINE_API void DoWork();

	/** Give the name for external event viewers
	* @return	the name to display in external event viewers
	*/
	static const TCHAR *Name()
	{
		return TEXT("FAsyncAudioDecompress");
	}
};

typedef FAsyncTask<FAsyncAudioDecompressWorker> FAsyncAudioDecompress;

