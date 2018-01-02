// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundEffectBase.h"
#include "IAmbisonicsMixer.generated.h"



class IAmbisonicsMixer;

typedef TSharedPtr<IAmbisonicsMixer, ESPMode::ThreadSafe> TAmbisonicsMixerPtr;

class FAudioDevice;

/** This is a class which should be overridden to provide users with settings to use for individual sounds */
UCLASS(config = Engine, abstract, editinlinenew, BlueprintType)
class ENGINE_API UAmbisonicsSubmixSettingsBase : public UObject
{
	GENERATED_BODY()
};

struct FAudioPluginInitializationParams;

struct FAmbisonicsEncoderInputData
{
	// The audio input buffer
	Audio::AlignedFloatBuffer* AudioBuffer;

	// Number of channels of the source audio buffer.
	int32 NumChannels;

	// if the input audio was already encoded to ambisonics,
	// this will point to the settings the audio was encoded with.
	// Otherwise, this will be a null pointer.
	UAmbisonicsSubmixSettingsBase* InputSettings;

	//Positions of each channel in the input AudioBuffer relative to listener.
	const TArray<FVector>* ChannelPositions;

	// Spatialization parameters.
	const FQuat ListenerRotation;
};

struct FAmbisonicsEncoderOutputData
{
	Audio::AlignedFloatBuffer& AudioBuffer;
};


struct FAmbisonicsDecoderInputData
{
	Audio::AlignedFloatBuffer* AudioBuffer;

	int32 NumChannels;

};

struct FAmbisonicsDecoderPositionalData
{
	int32 OutputNumChannels;

	const TArray<FVector>* OutputChannelPositions;
	
	FQuat ListenerRotation;
};

struct FAmbisonicsDecoderOutputData 
{
	Audio::AlignedFloatBuffer& AudioBuffer;
};

class IAmbisonicsMixer
{
public:
	/** Virtual destructor */
	virtual ~IAmbisonicsMixer()
	{
	}

	/**
	* Shuts down the audio plugin.
	*
	*/
	virtual void Shutdown()
	{
	}

	virtual void OnDeviceShutdown(FAudioDevice* AudioDevice)
	{
	}

	/** Returns the number of channels for a given format. */
	virtual int32 GetNumChannelsForAmbisonicsFormat(UAmbisonicsSubmixSettingsBase* InSettings) = 0;

	/** Called when a stream is opened. */
	virtual void OnOpenEncodingStream(const uint32 SourceId, UAmbisonicsSubmixSettingsBase* InSettings)
	{
	}

	/** Called when a stream is done playing and is released. */
	virtual void OnCloseEncodingStream(const uint32 SourceId)
	{
	}

	/** Encode the given source input data to ambisonics. */
	virtual void EncodeToAmbisonics(const uint32 SourceId, const FAmbisonicsEncoderInputData& InputData, FAmbisonicsEncoderOutputData& OutputData, UAmbisonicsSubmixSettingsBase* InParams)
	{
	}

	/** Open a decoding stream with the given stream id. */
	virtual void OnOpenDecodingStream(const uint32 StreamId, UAmbisonicsSubmixSettingsBase* InParams, FAmbisonicsDecoderPositionalData& SpecifiedOutputPositions)
	{
	}

	/** Closes the decoding stream. */
	virtual void OnCloseDecodingStream(const uint32 StreamId)
	{
	}

	/** Decodes the ambisonics data into interleaved audio. */
	virtual void DecodeFromAmbisonics(const uint32 StreamId, const FAmbisonicsDecoderInputData& InputData, FAmbisonicsDecoderPositionalData& SpecifiedOutputPositions, FAmbisonicsDecoderOutputData& OutputData)
	{
	}

	/*
	* Override this function to determine whether an audio buffer incoming from a different submix needs to be 
	* reencoded. If this returns true, a seperate encode/decode stream will be set up for the submix.
	* if this returns false, then the source submix's audio will be passed down directly.
	*/
	virtual bool ShouldReencodeBetween(UAmbisonicsSubmixSettingsBase* SourceSubmixSettings, UAmbisonicsSubmixSettingsBase* DestinationSubmixSettings)
	{
		return true;
	}

	/** Initializes the ambisonics plugin with the given buffer length. */
	virtual void Initialize(const FAudioPluginInitializationParams InitializationParams) = 0;

	virtual UClass* GetCustomSettingsClass()
	{
		return nullptr;
	}

	virtual UAmbisonicsSubmixSettingsBase* GetDefaultSettings()
	{
		return nullptr;
	}
};


namespace AmbisonicsStatics
{
	ENGINE_API TArray<FVector>* GetDefaultPositionMap(int32 NumChannels);
}