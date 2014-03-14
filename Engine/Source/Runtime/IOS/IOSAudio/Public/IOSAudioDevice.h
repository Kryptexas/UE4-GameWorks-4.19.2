// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	IOSAudioDevice.h: Unreal IOSAudio audio interface object.
 =============================================================================*/

#pragma once

#include "Engine.h"
#include "SoundDefinitions.h"
#include "AudioEffect.h"

/*------------------------------------------------------------------------------------
	Audio Framework system headers
 ------------------------------------------------------------------------------------*/
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>
#include <AVFoundation/AVAudioSession.h>

DECLARE_LOG_CATEGORY_EXTERN(LogIOSAudio, Log, All);

/*------------------------------------------------------------------------------------
	Dependencies, helpers & forward declarations.
------------------------------------------------------------------------------------*/

#define CHANNELS_PER_BUS 2

class FIOSAudioDevice;
class FIOSAudioEffectsManager;

enum ESoundFormat
{
	SoundFormat_Invalid,
	SoundFormat_LPCM,
	SoundFormat_ADPCM
};

struct FAudioBuffer
{
	int16* StreamBuffer;
	uint32 StreamReadCursor;
	uint32 SampleReadCursor;

	int32  Channel;
	int32  ChannelLock;
	bool   bFinished;
	bool   bLooped;
};

/**
 * IOSAudio implementation of FSoundBuffer, containing the wave data and format information.
 */
class FIOSAudioSoundBuffer : public FSoundBuffer
{
public:
	FIOSAudioSoundBuffer(FIOSAudioDevice* InAudioDevice, ESoundFormat InSoundFormat);
	virtual ~FIOSAudioSoundBuffer(void);

	/**
	 * Static function used to create a IOSAudio buffer and upload decompressed ogg vorbis data to.
	 *
	 * @param Wave			USoundWave to use as template and wave source
	 * @param AudioDevice	audio device to attach created buffer to
	 * @return FIOSAudioSoundBuffer pointer if buffer creation succeeded, NULL otherwise
	 */
	static FIOSAudioSoundBuffer* CreateNativeBuffer(FIOSAudioDevice* InAudioDevice, USoundWave* InWave);
	
	/**
	 * Static function used to create a buffer.
	 *
	 * @param InWave USoundWave to use as template and wave source
	 * @param AudioDevice audio device to attach created buffer to
	 * @return FIOSAudioSoundBuffer pointer if buffer creation succeeded, NULL otherwise
	 */
	static FIOSAudioSoundBuffer* Init(FIOSAudioDevice* IOSAudioDevice, USoundWave* InWave);
	
	/**
	 * Returns the size of this buffer in bytes.
	 *
	 * @return Size in bytes
	 */
	virtual int32 GetSize(void) OVERRIDE;

	/** Format of the sound referenced by this buffer */
	int32  SoundFormat;
	/** Address of PCM data in physical memory */
	int16* SampleData;
	int32  SampleRate;
	uint32 CompressedBlockSize;
	uint32 UncompressedBlockSize;
	uint32 BufferSize;
};

/**
 * IOSAudio implementation of FSoundSource, the interface used to play, stop and update sources
 */
class FIOSAudioSoundSource : public FSoundSource
{
public:
	/**
	 * Constructor
	 *
	 * @param	InAudioDevice	audio device this source is attached to
	 */
	FIOSAudioSoundSource(FIOSAudioDevice* InAudioDevice, uint32 InBusNumber);
	
	/** 
	 * Destructor
	 */
	~FIOSAudioSoundSource(void);
	
	/**
	 * Initializes a source with a given wave instance and prepares it for playback.
	 *
	 * @param	WaveInstance	wave instance being primed for playback
	 * @return	true			if initialization was successful, false otherwise
	 */
	virtual bool Init(FWaveInstance* WaveInstance) OVERRIDE;
	
	/**
	 * Updates the source specific parameter like e.g. volume and pitch based on the associated
	 * wave instance.	
	 */
	virtual void Update(void) OVERRIDE;
	
	/**  Plays the current wave instance. */
	virtual void Play(void) OVERRIDE;
	
	/** Stops the current wave instance and detaches it from the source. */
	virtual void Stop(void) OVERRIDE;
	
	/** Pauses playback of current wave instance. */
	virtual void Pause(void) OVERRIDE;
	
	/**
	 * Queries the status of the currently associated wave instance.
	 *
	 * @return	true if the wave instance/ source has finished playback and false if it is 
	 *			currently playing or paused.
	 */
	virtual bool IsFinished(void) OVERRIDE;

	/** Calculates the audio unit element of the input channel relative to the base bus number */
	AudioUnitElement GetAudioUnitElement(int32 Channel);

protected:
	bool AttachToAUGraph();
	bool DetachFromAUGraph();

	FIOSAudioDevice*      IOSAudioDevice;
	/** Cached sound buffer associated with currently bound wave instance. */
	FIOSAudioSoundBuffer* Buffer;
	/** A pair of sound buffers to allow notification when a sound loops. */
	FAudioBuffer          StreamBufferStates[CHANNELS_PER_BUS];
	uint32                StreamBufferSize;
	int32                 SampleRate;
	uint32                BusNumber;

private:
	void FreeResources();
	
	static OSStatus IOSAudioRenderCallback(void *InRefCon, AudioUnitRenderActionFlags *IOActionFlags,
	                                       const AudioTimeStamp *InTimeStamp, UInt32 InBusNumber,
	                                       UInt32 InNumberFrames, AudioBufferList *IOData);

	OSStatus IOSAudioRenderCallbackADPCM(AudioSampleType* OutData, UInt32 NumFrames, UInt32& NumFramesWritten, UInt32 Channel);
	OSStatus IOSAudioRenderCallbackLPCM (AudioSampleType* OutData, UInt32 NumFrames, UInt32& NumFramesWritten, UInt32 Channel);

	friend class FIOSAudioDevice;
};

/**
 * IOSAudio implementation of an Unreal audio device.
 */
class FIOSAudioDevice : public FAudioDevice
{
public:
	FIOSAudioDevice();
	virtual ~FIOSAudioDevice() { }
	
	virtual FName GetRuntimeFormat() OVERRIDE
	{
		static FName NAME_ADPCM(TEXT("ADPCM"));
		return NAME_ADPCM;
	}

	AUGraph GetAudioUnitGraph() const { return AudioUnitGraph; }
	AUNode GetMixerNode() const { return MixerNode; }
	AudioUnit GetMixerUnit() const { return MixerUnit; }

	/** Thread context management */
	static void ResumeContext();
	static void SuspendContext();
	
	virtual bool Exec( UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar = *GLog ) OVERRIDE;

protected:
	/** Starts up any platform specific hardware/APIs */
	virtual bool InitializeHardware() OVERRIDE;

	/** Shuts down any platform specific hardware/APIs */
	virtual void TeardownHardware() OVERRIDE;

	/** Lets the platform any tick actions */
	virtual void UpdateHardware() OVERRIDE;

	/** Creates a new platform specific sound source */
	virtual FAudioEffectsManager* CreateEffectsManager() OVERRIDE;

	/** Creates a new platform specific sound source */
	virtual FSoundSource* CreateSoundSource() OVERRIDE;

	/** Audio Session management */
	void GetHardwareSampleRate(double& OutSampleRate);
	bool SetHardwareSampleRate(const double& InSampleRate);
	bool SetAudioSessionActive(bool bActive);
	
private:
	void HandleError(const TCHAR* InLogOutput, bool bTeardown = false);

	/** Thread context management */
	static int32& GetSuspendCounter();

	AudioStreamBasicDescription MixerFormat;
	AUGraph                     AudioUnitGraph;
	AUNode                      OutputNode;
	AudioUnit                   OutputUnit;
	AUNode                      MixerNode;
	AudioUnit                   MixerUnit;
	FVector                     PlayerLocation;
	FVector                     PlayerFacing;
	FVector                     PlayerUp;
	FVector                     PlayerRight;
	int32                       NextBusNumber;

	friend class FIOSAudioSoundBuffer;
	friend class FIOSAudioSoundSource;
};
