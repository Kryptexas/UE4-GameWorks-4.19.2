//
// Copyright (C) Google Inc. 2017. All rights reserved.
//
#pragma once

#include "IAudioExtensionPlugin.h"
#include "ResonanceAudioCommon.h"

namespace ResonanceAudio
{
	class FResonanceAudioAmbisonicsMixer : public IAmbisonicsMixer
	{
	public:
		FResonanceAudioAmbisonicsMixer();

		// Begin IAmbisonicsMixer
		virtual void Shutdown() override;
		virtual int32 GetNumChannelsForAmbisonicsFormat(UAmbisonicsSubmixSettingsBase* InSettings) override;
		virtual void OnOpenEncodingStream(const uint32 StreamId, UAmbisonicsSubmixSettingsBase* InSettings) override;
		virtual void OnCloseEncodingStream(const uint32 StreamId) override;
		virtual void EncodeToAmbisonics(const uint32 StreamId, const FAmbisonicsEncoderInputData& InputData, FAmbisonicsEncoderOutputData& OutputData, UAmbisonicsSubmixSettingsBase* InParams) override;
		virtual void OnOpenDecodingStream(const uint32 StreamId, UAmbisonicsSubmixSettingsBase* InParams, FAmbisonicsDecoderPositionalData& SpecifiedOutputPositions) override;
		virtual void OnCloseDecodingStream(const uint32 StreamId) override;
		virtual void DecodeFromAmbisonics(const uint32 StreamId, const FAmbisonicsDecoderInputData& InputData, FAmbisonicsDecoderPositionalData& SpecifiedOutputPositions, FAmbisonicsDecoderOutputData& OutputData) override;
		virtual bool ShouldReencodeBetween(UAmbisonicsSubmixSettingsBase* SourceSubmixSettings, UAmbisonicsSubmixSettingsBase* DestinationSubmixSettings) override;
		virtual void Initialize(const FAudioPluginInitializationParams InitializationParams) override;
		virtual UClass* GetCustomSettingsClass() override;
		virtual UAmbisonicsSubmixSettingsBase* GetDefaultSettings() override;
		//~ IAmbisonicsMixer

		// This function is called by the ResonanceAudioPluginListener as soon as the ResonanceAudioApi is initialized.
		void SetResonanceAudioApi(vraudio::VrAudioApi* InResonanceAudioApi) { ResonanceAudioApi = InResonanceAudioApi; };

	private:
		// Map of which audio engine StreamIds map to which Resonance SourceIds.
		TMap<uint32, RaSourceId> ResonanceSourceIdMap;

		//Handle to the API context used by this plugin.
		vraudio::VrAudioApi* ResonanceAudioApi;
	};
}