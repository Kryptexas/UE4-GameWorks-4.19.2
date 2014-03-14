// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma  once 

/*------------------------------------------------------------------------------------
	Dependencies, helpers & forward declarations.
------------------------------------------------------------------------------------*/

class FSLESAudioDevice;

#include "Engine.h"
#include "SoundDefinitions.h"
#include "AudioEffect.h"

#include <SLES/OpenSLES.h>
#include "SLES/OpenSLES_Android.h"

DECLARE_LOG_CATEGORY_EXTERN(LogAndroidAudio,Log,VeryVerbose);

enum ESoundFormat
{
	SoundFormat_Invalid,
	SoundFormat_PCM,
	SoundFormat_PCMRT
};

struct SLESAudioBuffer
{
	uint8 *AudioData;
	int32 AudioDataSize;
	int32 ReadCursor;
};

/**
 * OpenSLES implementation of FSoundBuffer, containing the wave data and format information.
 */
class FSLESSoundBuffer : public FSoundBuffer 
{
public:
	/** 
	 * Constructor
	 *
	 * @param AudioDevice	audio device this sound buffer is going to be attached to.
	 */
	FSLESSoundBuffer( FSLESAudioDevice* AudioDevice );
	
	/**
	 * Destructor 
	 * 
	 * Frees wave data and detaches itself from audio device.
 	 */
	~FSLESSoundBuffer( void );

	/**
	 * Static function used to create a buffer and dynamically upload decompressed ogg vorbis data to.
	 *
	 * @param InWave		USoundWave to use as template and wave source
	 * @param AudioDevice	audio device to attach created buffer to
	 * @return FCoreAudioSoundBuffer pointer if buffer creation succeeded, NULL otherwise
	 */
	static FSLESSoundBuffer* CreateQueuedBuffer( FSLESAudioDevice* AudioDevice, USoundWave* Wave );

	/**
	 * Static function used to create an Audio buffer and upload decompressed ogg vorbis data to.
	 *
	 * @param Wave			USoundWave to use as template and wave source
	 * @param AudioDevice	audio device to attach created buffer to
	 * @return FCoreAudioSoundBuffer pointer if buffer creation succeeded, NULL otherwise
	 */
	static FSLESSoundBuffer* CreateNativeBuffer( FSLESAudioDevice* AudioDevice, USoundWave* Wave );

	/**
	 * Static function used to create a buffer.
	 *
	 * @param	InWave				USoundWave to use as template and wave source
	 * @param	AudioDevice			Audio device to attach created buffer to
	 * @return	FALSoundBuffer pointer if buffer creation succeeded, NULL otherwise
	 */
	static FSLESSoundBuffer* Init(  FSLESAudioDevice* ,USoundWave* );

	/**
	 * Decompresses a chunk of compressed audio to the destination memory
	 *
	 * @param Destination		Memory to decompress to
	 * @param bLooping			Whether to loop the sound seamlessly, or pad with zeroes
	 * @return					Whether the sound looped or not
	 */
	bool ReadCompressedData( uint8* Destination, bool bLooping );

	/**
	 * Returns the size of this buffer in bytes.
	 *
	 * @return Size in bytes
	 */
	int GetSize( void ) 
	{ 
		return( BufferSize ); 
	}

	/** 
	 * Returns the number of channels for this buffer
	 */
	int GetNumChannels( void ) 
	{ 
		return( NumChannels ); 
	}
	
		
	/** Audio device this buffer is attached to */
	FSLESAudioDevice*			AudioDevice;
	/** Data */
	uint8*					AudioData;
	/** Resource ID of associated USoundeWave */
	int						ResourceID;
	/** Human readable name of resource, most likely name of UObject associated during caching. */
	FString					ResourceName;

	/** Number of bytes stored in OpenAL, or the size of the ogg vorbis data */
	int						BufferSize;
	/** The number of channels in this sound buffer - should be directly related to InternalFormat */
	int						NumChannels;
	/** Sample rate of the ogg vorbis data - typically 44100 or 22050 */
	int						SampleRate;

	/** Wrapper to handle the decompression of vorbis code */
	class FVorbisAudioInfo*		DecompressionState;

	/** Format of data to be received by the source */
	ESoundFormat Format;
	
};

/**
 * OpenSLES implementation of FSoundSource, the interface used to play, stop and update sources
 */
class FSLESSoundSource : public FSoundSource
{
public:
	/**
	 * Constructor
	 *
	 * @param	InAudioDevice	audio device this source is attached to
	 */
	FSLESSoundSource( class FAudioDevice* InAudioDevice );

	/** 
	 * Destructor
	 */
	~FSLESSoundSource( void );

	/**
	 * Initializes a source with a given wave instance and prepares it for playback.
	 *
	 * @param	WaveInstance	wave instance being primed for playback
	 * @return	TRUE			if initialization was successful, FALSE otherwise
	 */
	virtual bool Init( FWaveInstance* WaveInstance );

	/**
	 * Updates the source specific parameter like e.g. volume and pitch based on the associated
	 * wave instance.	
	 */
	virtual void Update( void );

	/**
	 * Plays the current wave instance.	
	 */
	virtual void Play( void );

	/**
	 * Stops the current wave instance and detaches it from the source.	
	 */
	virtual void Stop( void );

	/**
	 * Pauses playback of current wave instance.
	 */
	virtual void Pause( void );

	/**
	 * Queries the status of the currently associated wave instance.
	 *
	 * @return	TRUE if the wave instance/ source has finished playback and FALSE if it is 
	 *			currently playing or paused.
	 */
	virtual bool IsFinished( void );

	/** 
	 * Returns TRUE if source has finished playing
	 */
	bool IsSourceFinished( void );

	/**
	 * Used to requeue a looping sound when the completion callback fires.
	 *
	 */
	void OnRequeueBufferCallback( SLAndroidSimpleBufferQueueItf InQueueInterface );


protected:

	friend class FSLESAudioDevice;
	FSLESSoundBuffer*		Buffer;
	FSLESAudioDevice*		Device;
	
	// OpenSL interface objects
	SLObjectItf						SL_PlayerObject;
	SLPlayItf						SL_PlayerPlayInterface;
	SLAndroidSimpleBufferQueueItf	SL_PlayerBufferQueue;
	SLVolumeItf						SL_VolumeInterface;

	/** Which sound buffer should be written to next - used for double buffering. */
	bool						bStreamedSound;
	/** A pair of sound buffers for real-time decoding */
	SLESAudioBuffer			AudioBuffers[2];
	/** Set when we wish to let the buffers play themselves out */
	bool						bBuffersToFlush;

	int32						BufferInUse;
	bool						bHasLooped;

	bool CreatePlayer();
	void DestroyPlayer();
	void ReleaseResources();
	bool EnqueuePCMBuffer( bool bLoop);
	bool EnqueuePCMRTBuffer( bool bLoop);
};

/**
 * OpenSLES implementation of an Unreal audio device.
 */
class FSLESAudioDevice : public FAudioDevice
{
public: 
	FSLESAudioDevice() {} 
	virtual ~FSLESAudioDevice() {} 

	virtual FName GetRuntimeFormat() OVERRIDE
	{
		static FName NAME_OGG(TEXT("OGG"));
		return NAME_OGG;
	}

	/** Starts up any platform specific hardware/APIs */
	virtual bool InitializeHardware() OVERRIDE;
 
	/**
	 * Update the audio device and calculates the cached inverse transform later
	 * on used for spatialization.
	 *
	 * @param	Realtime	whether we are paused or not
	 */
	virtual void Update( bool bGameTicking );

	/** 
	 * Lists all the loaded sounds and their memory footprint
	 */
	virtual void ListSounds( const TCHAR* Cmd, FOutputDevice& Ar );

	/**
	 * Frees the bulk resource data associated with this SoundNodeWave.
	 *
	 * @param	SoundNodeWave	wave object to free associated bulk data
	 */
	virtual void FreeResource( USoundWave* SoundWave );


protected:

	virtual FSoundSource* CreateSoundSource() OVERRIDE; 


	/**
	 * Tears down audio device by stopping all sounds, removing all buffers,  destroying all sources, ... Called by both Destroy and ShutdownAfterError
	 * to perform the actual tear down.
	 */
	void Teardown( void );

	// Variables.

	/** The name of the OpenSL Device to open - defaults to "Generic Software" */
	FString										DeviceName;
	/** All loaded resident buffers */
	TArray<FSLESSoundBuffer*>					Buffers;
	/** Map from resource ID to sound buffer */
	TMap<int, FSLESSoundBuffer*>				WaveBufferMap;
	/** Next resource ID value used for registering USoundWave objects */
	int											NextResourceID;

	// SL specific

	
	// engine interfaces
	SLObjectItf									SL_EngineObject;
	SLEngineItf									SL_EngineEngine;
	SLObjectItf									SL_OutputMixObject;
	
	SLint32										SL_VolumeMax;
	SLint32										SL_VolumeMin;
	
	friend class FSLESSoundBuffer;
	friend class FSLESSoundSource;
};
