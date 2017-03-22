// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IAudioExtensionPlugin.h"
#include "Sound/SoundEffectSubmix.h"
#include "Sound/SoundEffectPreset.h"
#include "PhononCommon.h"
#include "phonon.h"
#include "PhononReverb.generated.h"

namespace Phonon
{
	class FPhononReverbSource
	{
	public:
		FPhononReverbSource()
			: ConvolutionEffect(nullptr)
		{}

		IPLhandle ConvolutionEffect;
		IPLAudioBuffer InBuffer;
	};

	class FPhononReverb : public IAudioReverb
	{
	public:
		FPhononReverb();
		~FPhononReverb();

		virtual void Initialize(const int32 SampleRate, const int32 NumSources, const int32 FrameSize) override;
		virtual void SetReverbSettings(const uint32 SourceId, const uint32 AudioComponentUserId, UReverbPluginSourceSettingsBase* InSettings) override;
		virtual class FSoundEffectSubmix* GetEffectSubmix(class USoundSubmix* Submix) override;
		virtual void ProcessSourceAudio(const FAudioPluginSourceInputData& InputData, FAudioPluginSourceOutputData& OutputData) override;
		
		void ProcessMixedAudio(const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData);
		void SetEnvironmentalRenderer(IPLhandle EnvironmentalRenderer);
		void UpdateListener(const FVector& Position, const FVector& Forward, const FVector& Up);

	private:
		IPLhandle EnvironmentalRenderer;
		IPLhandle BinauralRenderer;
		IPLhandle IndirectBinauralEffect;
		IPLhandle ReverbConvolutionEffect;
		IPLAudioBuffer DryBuffer;

		IPLAudioBuffer IndirectOutBuffer;
		int32 AmbisonicsChannels;
		float** IndirectOutDeinterleaved;
		TArray<float> IndirectOutArray;

		IPLAudioBuffer IndirectIntermediateBuffer;
		TArray<float> IndirectIntermediateArray;

		IPLAudioFormat InputAudioFormat;
		IPLAudioFormat ReverbInputAudioFormat;
		IPLAudioFormat IndirectOutputAudioFormat;
		IPLAudioFormat BinauralOutputAudioFormat;

		FCriticalSection ListenerCriticalSection;
		IPLVector3 ListenerPosition;
		IPLVector3 ListenerForward;
		IPLVector3 ListenerUp;

		IPLRenderingSettings RenderingSettings;

		TArray<FPhononReverbSource> PhononReverbSources;
	};
}

USTRUCT()
struct FSubmixEffectReverbPluginSettings
{
	GENERATED_USTRUCT_BODY()
};

class FSubmixEffectReverbPlugin : public FSoundEffectSubmix
{
public:
	FSubmixEffectReverbPlugin()
		: PhononReverbPlugin(nullptr)
	{}

	virtual void Init(const FSoundEffectSubmixInitData& InSampleRate) override;
	virtual uint32 GetDesiredInputChannelCountOverride() const override;
	virtual void OnProcessAudio(const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData) override;
	virtual void OnPresetChanged() override;

	void SetPhononReverbPlugin(Phonon::FPhononReverb* InPhononReverbPlugin)
	{
		PhononReverbPlugin = InPhononReverbPlugin;
	}

private:
	Phonon::FPhononReverb* PhononReverbPlugin;
};

UCLASS()
class USubmixEffectReverbPluginPreset : public USoundEffectSubmixPreset
{
	GENERATED_BODY()

public:
	EFFECT_PRESET_METHODS_NO_ASSET_ACTIONS(SubmixEffectReverbPlugin)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SubmixEffectPreset)
	FSubmixEffectReverbPluginSettings Settings;
};