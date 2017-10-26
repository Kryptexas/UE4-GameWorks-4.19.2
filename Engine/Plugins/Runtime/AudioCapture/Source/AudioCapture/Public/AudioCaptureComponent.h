// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AudioCapture.h"
#include "Components/SynthComponent.h"
#include "AudioCaptureComponent.generated.h"

UCLASS(ClassGroup = Synth, meta = (BlueprintSpawnableComponent))
class AUDIOCAPTURE_API UAudioCaptureComponent : public USynthComponent
{
	GENERATED_BODY()

	UAudioCaptureComponent(const FObjectInitializer& ObjectInitializer);

	//~ Begin USynthComponent interface
	virtual bool Init(int32& SampleRate) override;
	virtual void OnStart() override;
	virtual void OnStop() override;
	virtual void OnGenerateAudio(float* OutAudio, int32 NumSamples) override;
	virtual void OnBeginGenerate() override;
	virtual void OnEndGenerate() override;
	//~ End USynthComponent interface

	//~ Begin UObject interface
	virtual void BeginDestroy();
	virtual bool IsReadyForFinishDestroy() override;
	virtual void FinishDestroy() override;
	//~ End UObject interface

private:

	Audio::FAudioCaptureSynth CaptureSynth;
	TArray<float> CaptureAudioData;
	int32 CaptureAudioDataSamples;

	bool bSuccessfullyInitialized;
	bool bIsCapturing;
	bool bIsStreamOpen;
	int32 CaptureChannels;
	int32 FramesSinceStarting;
	int32 ReadSampleIndex;
	FThreadSafeBool bIsDestroying;
	FThreadSafeBool bIsReadyForForFinishDestroy;
};