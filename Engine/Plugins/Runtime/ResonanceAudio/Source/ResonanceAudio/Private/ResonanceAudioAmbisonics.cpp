//
// Copyright (C) Google Inc. 2017. All rights reserved.
//
#include "ResonanceAudioAmbisonics.h"
#include "ResonanceAudioAmbisonicsSettings.h"

namespace ResonanceAudio
{

	const int32 DefaultNumChannels = 4;


	int32 GetNumChannelsForAmbisonicsOrder(EAmbisonicsOrder InOrder)
	{
		switch (InOrder)
		{
			case EAmbisonicsOrder::SecondOrder:
			{
				return 9;
			}

			case EAmbisonicsOrder::ThirdOrder:
			{
				return 16;
			}

			case EAmbisonicsOrder::FirstOrder:
			default:
			{
				return 4;
			}
		}
	}

	FResonanceAudioAmbisonicsMixer::FResonanceAudioAmbisonicsMixer()
		: ResonanceAudioApi(nullptr)
	{
	}

	void FResonanceAudioAmbisonicsMixer::Shutdown()
	{
	}

	int32 FResonanceAudioAmbisonicsMixer::GetNumChannelsForAmbisonicsFormat(UAmbisonicsSubmixSettingsBase* InSettings)
	{
		UResonanceAudioAmbisonicsSettings* CastedSettings = Cast<UResonanceAudioAmbisonicsSettings>(InSettings);

		if (CastedSettings != nullptr)
		{
			const EAmbisonicsOrder Order = CastedSettings->AmbisonicsOrder;
			return GetNumChannelsForAmbisonicsOrder(Order);
		}
		else
		{
			return 4;
		}
	}

	void FResonanceAudioAmbisonicsMixer::OnOpenEncodingStream(const uint32 StreamId, UAmbisonicsSubmixSettingsBase* InSettings)
	{
		UE_LOG(LogResonanceAudio, Warning, TEXT("Encoding into an ambisonics is not currently supported by the Resonance Audio plugin."));
	}

	void FResonanceAudioAmbisonicsMixer::OnCloseEncodingStream(const uint32 SourceId)
	{
		//TODO: Implement.
	}

	void FResonanceAudioAmbisonicsMixer::EncodeToAmbisonics(const uint32 SourceId, const FAmbisonicsEncoderInputData& InputData, FAmbisonicsEncoderOutputData& OutputData, UAmbisonicsSubmixSettingsBase* InParams)
	{
		//TODO: Implement.
	}

	void FResonanceAudioAmbisonicsMixer::OnOpenDecodingStream(const uint32 StreamId, UAmbisonicsSubmixSettingsBase* InSettings, FAmbisonicsDecoderPositionalData& SpecifiedOutputPositions)
	{
		if (ResonanceAudioApi == nullptr)
		{
			return;
		}

		int32 NumChannels = 4;

		UResonanceAudioAmbisonicsSettings* MyAmbisonicsSettings = Cast<UResonanceAudioAmbisonicsSettings>(InSettings);
		if(MyAmbisonicsSettings != nullptr)
		{
			NumChannels = GetNumChannelsForAmbisonicsOrder(MyAmbisonicsSettings->AmbisonicsOrder);
		}

		ResonanceSourceIdMap.Add(StreamId, ResonanceAudioApi->CreateAmbisonicSource(NumChannels));
	}

	void FResonanceAudioAmbisonicsMixer::OnCloseDecodingStream(const uint32 StreamId)
	{
		if (ResonanceAudioApi == nullptr)
		{
			return;
		}

		RaSourceId* InStreamId = ResonanceSourceIdMap.Find(StreamId);
		if (InStreamId != nullptr)
		{
			ResonanceAudioApi->DestroySource(*InStreamId);
			ResonanceSourceIdMap.Remove(*InStreamId);
		}
		else
		{
			checkf(false, TEXT("Tried to close a stream we did not open."));
		}
	}

	void FResonanceAudioAmbisonicsMixer::DecodeFromAmbisonics(const uint32 StreamId, const FAmbisonicsDecoderInputData& InputData, FAmbisonicsDecoderPositionalData& SpecifiedOutputPositions, FAmbisonicsDecoderOutputData& OutputData)
	{
		if (ResonanceAudioApi == nullptr)
		{
			return;
		}

		
		//Get the source ID for this Ambisonics file.
		RaSourceId* ThisStream = ResonanceSourceIdMap.Find(StreamId);

		//If it doesn't exist already, create a new source.
		if (ThisStream == nullptr)
		{
			ThisStream = &ResonanceSourceIdMap.Add(StreamId, ResonanceAudioApi->CreateAmbisonicSource(InputData.NumChannels));
		}
			
		int32 NumFrames = InputData.AudioBuffer->Num() / InputData.NumChannels;
		float* InputAudio = InputData.AudioBuffer->GetData();

		ResonanceAudioApi->SetInterleavedBuffer(*ThisStream, InputAudio, InputData.NumChannels, NumFrames);
		
	}

	bool FResonanceAudioAmbisonicsMixer::ShouldReencodeBetween(UAmbisonicsSubmixSettingsBase* SourceSubmixSettings, UAmbisonicsSubmixSettingsBase* DestinationSubmixSettings)
	{
		return true;
	}

	void FResonanceAudioAmbisonicsMixer::Initialize(const FAudioPluginInitializationParams InitializationParams)
	{
		// Nothing to do here for now.
	}

	UClass* FResonanceAudioAmbisonicsMixer::GetCustomSettingsClass()
	{
		return UResonanceAudioAmbisonicsSettings::StaticClass();
	}

	UAmbisonicsSubmixSettingsBase* FResonanceAudioAmbisonicsMixer::GetDefaultSettings()
	{
		static UResonanceAudioAmbisonicsSettings* DefaultAmbisonicsSettings = nullptr;
		
		if (DefaultAmbisonicsSettings == nullptr)
		{
			DefaultAmbisonicsSettings = NewObject<UResonanceAudioAmbisonicsSettings>();
			DefaultAmbisonicsSettings->AmbisonicsOrder = EAmbisonicsOrder::FirstOrder;
			DefaultAmbisonicsSettings->AddToRoot();
		}

		return DefaultAmbisonicsSettings;
	}

}