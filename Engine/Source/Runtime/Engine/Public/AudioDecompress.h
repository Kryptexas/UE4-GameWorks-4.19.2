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
 * Helper class to parse ogg vorbis data
 */
class FVorbisAudioInfo
{
public:
	ENGINE_API FVorbisAudioInfo( void );
	ENGINE_API ~FVorbisAudioInfo( void );

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
#if WITH_OGGVORBIS
	ENGINE_API bool ReadCompressedInfo( uint8* InSrcBufferData, uint32 InSrcBufferDataSize, struct FSoundQualityInfo* QualityInfo );
#else
	ENGINE_API bool ReadCompressedInfo( uint8* InSrcBufferData, uint32 InSrcBufferDataSize, struct FSoundQualityInfo* QualityInfo )
	{
		return( false );
	}
#endif

	/** 
	 * Decompresses ogg data to raw PCM data. 
	 * 
	 * @param	PCMData		where to place the decompressed sound
	 * @param	bLooping	whether to loop the sound by seeking to the start, or pad the buffer with zeroes
	 * @param	BufferSize	number of bytes of PCM data to create
	 *
	 * @return	bool		true if the end of the data was reached (for both single shot and looping sounds)
	 */
#if WITH_OGGVORBIS
	ENGINE_API bool ReadCompressedData( const uint8* Destination, bool bLooping, uint32 BufferSize = 0 );
#else
	ENGINE_API bool ReadCompressedData( const uint8* Destination, bool bLooping, uint32 BufferSize = 0 )
	{
		return( false );
	}
#endif

#if WITH_OGGVORBIS
	ENGINE_API void SeekToTime( const float SeekTime );
#else
	void SeekToTime( const float SeekTime ) { }
#endif

	/** 
	 * Decompress an entire ogg data file to a TArray
	 */
#if WITH_OGGVORBIS
	ENGINE_API void ExpandFile( uint8* DstBuffer, struct FSoundQualityInfo* QualityInfo );
#else
	ENGINE_API void ExpandFile( uint8* DstBuffer, struct FSoundQualityInfo* QualityInfo )
	{
	}
#endif

	/** 
	 * Sets ogg to decode to half-rate
	 * 
	 * @param	Resource		Info about vorbis data
	 */
#if WITH_OGGVORBIS
	ENGINE_API void EnableHalfRate( bool HalfRate );
#else
	ENGINE_API void EnableHalfRate( bool HalfRate )
	{
	}
#endif

	struct FVorbisFileWrapper* VFWrapper;
	const uint8*		SrcBufferData;
	uint32			SrcBufferDataSize;
	uint32			BufferOffset;
};



/**
 * Asynchronous vorbis decompression
 */
class FAsyncVorbisDecompressWorker : public FNonAbandonableTask
{
protected:
	class USoundWave*		Wave;

public:
	/**
	 * Async decompression of vorbis data
	 *
	 * @param	InWave		Wave data to decompress
	 */
	FAsyncVorbisDecompressWorker( USoundWave* InWave)
		: Wave(InWave)
	{
	}

	/**
	 * Performs the async vorbis decompression
	 */
	ENGINE_API void DoWork();

	/** Give the name for external event viewers
	* @return	the name to display in external event viewers
	*/
	static const TCHAR *Name()
	{
		return TEXT("FAsyncVorbisDecompress");
	}
};

typedef FAsyncTask<FAsyncVorbisDecompressWorker> FAsyncVorbisDecompress;

