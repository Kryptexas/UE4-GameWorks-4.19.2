// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Sound/SoundEffectPreset.h"
#include "Sound/SoundEffectBase.h"

#include "SoundEffectSource.generated.h"

class FSoundEffectSource;
class FSoundEffectBase;

/** This is here to make sure users don't mix up source and submix effects in the editor. Asset sorting, drag-n-drop, etc. */
UCLASS(config = Engine, abstract, editinlinenew, BlueprintType)
class ENGINE_API USoundEffectSourcePreset : public USoundEffectPreset
{
	GENERATED_BODY()
};

/** Struct which has data needed to initialize the source effect. */
struct FSoundEffectSourceInitData
{
	float SampleRate;
	int32 NumSourceChannels;
	float SourceDuration;
	double AudioClock;
};

/** Struct which has data to initialize the source effect. */
struct FSoundEffectSourceInputData
{
	void* PresetData;
	TArray<float> AudioFrame;
	FVector SourcePosition;
	FVector LeftChannelPosition;
	FVector RightChannelPosition;
	FVector ListenerPosition;
	float CurrentVolume;
	float CurrentPitch;
	float CurrentPlayTime;
	float Duration;
	float Distance;
	double AudioClock;
	int32 CurrentLoopCount;
	int32 MaxLoopCount;
	uint8 bLooping : 1;
	uint8 bIsSpatialized : 1;
};

struct FSoundEffectSourceOutputData
{
	TArray<float> AudioFrame;
};

class ENGINE_API FSoundEffectSource : public FSoundEffectBase
{
public:
	FSoundEffectSource() {}
	virtual ~FSoundEffectSource() {}

	/** Called on an audio effect at initialization on main thread before audio processing begins. */
	virtual void Init(const FSoundEffectSourceInitData& InSampleRate) = 0;

	/** Is called when a preset is changed. Allows subclasses to cast the preset object to the derived type and get their data. */
	virtual void SetPreset(USoundEffectSourcePreset* InPreset) = 0;

	/** Process the input block of audio. Called on audio thread. */
	virtual void ProcessAudio(const FSoundEffectSourceInputData& InData, FSoundEffectSourceOutputData& OutData) = 0;
	
protected:

	// Allow FAudioMixerSubmix to call ProcessAudio
	friend class FMixerSubmix;
};

