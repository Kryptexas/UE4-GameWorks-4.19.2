// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DSP/DelayStereo.h"
#include "Sound/SoundEffectSource.h"
#include "SourceEffectStereoDelay.generated.h"

UENUM(BlueprintType)
enum class EStereoDelaySourceEffect : uint8
{
	Normal = 0,
	Cross,
	PingPong,
	Count UMETA(Hidden)
};

USTRUCT()
struct SYNTHESIS_API FSourceEffectStereoDelaySettings
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SourceEffect|Preset")
	EStereoDelaySourceEffect DelayMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SourceEffect|Preset")
	float DelayTimeMsec;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SourceEffect|Preset")
	float Feedback;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SourceEffect|Preset")
	float DelayRatio;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SourceEffect|Preset")
	float WetLevel;
};

class SYNTHESIS_API FSourceEffectStereoDelay : public FSoundEffectSource
{
public:
	// Called on an audio effect at initialization on main thread before audio processing begins.
	virtual void Init(const FSoundEffectSourceInitData& InSampleRate) override;
	
	// Called when an audio effect preset is changed
	virtual void SetPreset(USoundEffectSourcePreset* InPreset) override;

	// Process the input block of audio. Called on audio thread.
	virtual void ProcessAudio(const FSoundEffectSourceInputData& InData, FSoundEffectSourceOutputData& OutData) override;

protected:
	Audio::FDelayStereo DelayStereo;
};

UCLASS()
class SYNTHESIS_API USourceEffectStereoDelayPreset : public USoundEffectSourcePreset
{
	GENERATED_BODY()

public:
	EFFECT_PRESET_METHODS(SourceEffectStereoDelay)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SourceEffect|Preset")
	FSourceEffectStereoDelaySettings Settings;
};
