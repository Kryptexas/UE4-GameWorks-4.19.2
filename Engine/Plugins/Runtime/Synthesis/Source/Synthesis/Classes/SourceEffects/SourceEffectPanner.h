// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sound/SoundEffectSource.h"
#include "SourceEffectPanner.generated.h"

USTRUCT(BlueprintType)
struct SYNTHESIS_API FSourceEffectPannerSettings
{
	GENERATED_USTRUCT_BODY()

	// The pan of the source. -1.0 means left, 0.0 means center, 1.0 means right.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SourceEffect|Preset", meta = (ClampMin = "-1.0", ClampMax = "1.0", UIMin = "-1.0", UIMax = "1.0"))
	float Pan;

	FSourceEffectPannerSettings()
		: Pan(0.0f)
	{}
};

class SYNTHESIS_API FSourceEffectPanner : public FSoundEffectSource
{
public:
	// Called on an audio effect at initialization on main thread before audio processing begins.
	virtual void Init(const FSoundEffectSourceInitData& InSampleRate) override;
	
	// Called when an audio effect preset is changed
	virtual void OnPresetChanged() override;

	// Process the input block of audio. Called on audio thread.
	virtual void ProcessAudio(const FSoundEffectSourceInputData& InData, FSoundEffectSourceOutputData& OutData) override;

protected:
	// The pan value of the source effect
	float PanValue;
};

UCLASS(ClassGroup = AudioSourceEffect, meta = (BlueprintSpawnableComponent))
class SYNTHESIS_API USourceEffectPannerPreset : public USoundEffectSourcePreset
{
	GENERATED_BODY()

public:
	EFFECT_PRESET_METHODS(SourceEffectPanner)

	virtual FColor GetPresetColor() const override { return FColor(127.0f, 155.0f, 101.0f); }

	UFUNCTION(BlueprintCallable, Category = "Audio|Effects")
	void SetSettings(const FSourceEffectPannerSettings& InSettings);
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio|Effects")
	FSourceEffectPannerSettings Settings;
};
