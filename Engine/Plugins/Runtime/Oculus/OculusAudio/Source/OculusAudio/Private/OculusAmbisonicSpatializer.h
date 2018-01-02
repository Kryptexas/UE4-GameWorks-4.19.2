// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IAudioExtensionPlugin.h"
#include "AudioEffect.h"
#include "DSP/Dsp.h"
#include "Sound/SoundEffectSubmix.h"
#include "OVR_Audio.h"
#include "OculusAmbisonicsSettings.h"

class FOculusAmbisonicsMixer : public IAmbisonicsMixer
{
public:
	FOculusAmbisonicsMixer();

	// Begin IAmbisonicsMixer
	virtual void Shutdown() override;
	virtual int32 GetNumChannelsForAmbisonicsFormat(UAmbisonicsSubmixSettingsBase* InSettings) override;
	virtual void OnOpenEncodingStream(const uint32 SourceId, UAmbisonicsSubmixSettingsBase* InSettings) override;
	virtual void OnCloseEncodingStream(const uint32 SourceId) override;
	virtual void EncodeToAmbisonics(const uint32 SourceId, const FAmbisonicsEncoderInputData& InputData, FAmbisonicsEncoderOutputData& OutputData, UAmbisonicsSubmixSettingsBase* InParams) override;
	virtual void OnOpenDecodingStream(const uint32 StreamId, UAmbisonicsSubmixSettingsBase* InParams, FAmbisonicsDecoderPositionalData& SpecifiedOutputPositions) override;
	virtual void OnCloseDecodingStream(const uint32 StreamId) override;
	virtual void DecodeFromAmbisonics(const uint32 StreamId, const FAmbisonicsDecoderInputData& InputData, FAmbisonicsDecoderPositionalData& SpecifiedOutputPositions, FAmbisonicsDecoderOutputData& OutputData) override;
	virtual bool ShouldReencodeBetween(UAmbisonicsSubmixSettingsBase* SourceSubmixSettings, UAmbisonicsSubmixSettingsBase* DestinationSubmixSettings) override;
	virtual void Initialize(const FAudioPluginInitializationParams InitializationParams) override;
	virtual UClass* GetCustomSettingsClass() override;
	virtual UAmbisonicsSubmixSettingsBase* GetDefaultSettings() override;
	//~ IAmbisonicsMixer

private:
	ovrAudioContext Context;
	TMap<uint32, ovrAudioAmbisonicStream> OpenStreams;
	int32 SampleRate;
	int32 BufferLength;
};