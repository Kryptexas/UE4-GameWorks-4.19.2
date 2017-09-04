// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#include "SubmixEffects/SubmixEffectTapDelay.h"

DEFINE_LOG_CATEGORY(LogTapDelay);

static void GetMultichannelPan(const float& Input, const float& Angle, const float& Gain, const int32& NumChannels, float* DestinationFrame)
{
	if (NumChannels == 2)
	{
		float LeftGain;
		float RightGain;
		Audio::GetStereoPan(Angle / -90.0f, LeftGain, RightGain);

		DestinationFrame[0] += LeftGain * Gain * Input;
		DestinationFrame[1] += RightGain * Gain * Input;
	}
	else
	{
		// TODO: improve multichannel pan to take into account the speaker positions in audio mixer
		for (int32 Channel = 0; Channel < NumChannels; Channel++)
		{
			DestinationFrame[Channel] += Gain * Input / ((float)NumChannels);
		}
	}
}

static void MixToMono(const float* InputFrame, const int32& NumChannels, float& Output)
{
	Output = 0.0f;

	for (int32 Channel = 0; Channel < NumChannels; Channel++)
	{
		Output += InputFrame[Channel];
	}
}

// Static id to get unique tap Ids
static int32 TapIdCount = 0;

FTapDelayInfo::FTapDelayInfo()
	: TapLineMode(ETapLineMode::Panning)
	, DelayLength(1000.0f)
	, Gain(-3.0f)
	, OutputChannel(0)
	, PanInDegrees(0.0f)
	, TapId(TapIdCount++)
{}

FSubmixEffectTapDelay::FSubmixEffectTapDelay()
	: SampleRate(0.0f)
	, MaxDelayLineLength(10000.0f)
	, TapIncrementsRemaining(0)
{
}

FSubmixEffectTapDelay::~FSubmixEffectTapDelay()
{
}

void FSubmixEffectTapDelay::Init(const FSoundEffectSubmixInitData& InData)
{
	SampleRate = InData.SampleRate;
	DelayLine.Init(SampleRate, MaxDelayLineLength / 1000.0f);
}

void FSubmixEffectTapDelay::OnProcessAudio(const FSoundEffectSubmixInputData& InData, FSoundEffectSubmixOutputData& OutData)
{
	UpdateParameters();
	check(InData.NumChannels == OutData.NumChannels);

	// Cache all pointers first to avoid bounds checks on dereferences:
	const float* InBuffer = InData.AudioBuffer->GetData();
	float* OutBuffer = OutData.AudioBuffer->GetData();

	const FTapDelayInfo* TapsPointer = TargetTaps.GetData();
	FTapDelayInterpolationInfo* CurrentTapsPtr = CurrentTaps.GetData();
	const int32 NumTaps = TargetTaps.Num();

	// If we have no taps to render, short circuit.
	if (!NumTaps)
	{
		return;
	}

	int32 InputBufferIndex = 0;
	for (int32 OutputBufferIndex = 0; OutputBufferIndex < OutData.AudioBuffer->Num(); OutputBufferIndex += OutData.NumChannels)
	{
		// Sum into this sample from each tap.
		for (int32 TapIndex = 0; TapIndex < NumTaps; TapIndex++)
		{
			const FTapDelayInfo& TargetTap = TapsPointer[TapIndex];
			FTapDelayInterpolationInfo& CurrentTap = CurrentTapsPtr[TapIndex];

			if (TargetTap.TapLineMode == ETapLineMode::Panning || TargetTap.TapLineMode == ETapLineMode::Disabled)
			{
				GetMultichannelPan(DelayLine.ReadDelayAt(CurrentTap.GetLengthValue()), TargetTap.PanInDegrees, CurrentTap.GetGainValue(), OutData.NumChannels, &(OutBuffer[OutputBufferIndex]));
			}
			else
			{
				const int32 BufferIndex = OutputBufferIndex + (TargetTap.OutputChannel % OutData.NumChannels);
				OutBuffer[BufferIndex] += DelayLine.ReadDelayAt(CurrentTap.GetLengthValue()) * CurrentTap.GetGainValue();
			}
		}
		// Finally, write our input into the delay line.
		float Input;
		MixToMono(&InBuffer[InputBufferIndex], InData.NumChannels, Input);
		DelayLine.WriteDelayAndInc(Input);
		InputBufferIndex += InData.NumChannels;
	}
}

void FSubmixEffectTapDelay::OnPresetChanged()
{
	GET_EFFECT_SETTINGS(SubmixEffectTapDelay);

	FSubmixEffectTapDelaySettings NewSettings;
	NewSettings = Settings;

	SetEffectParameters(NewSettings);
}

void FSubmixEffectTapDelay::SetEffectParameters(const FSubmixEffectTapDelaySettings& InTapEffectParameters)
{
	Params.SetParams(InTapEffectParameters);
}

void FSubmixEffectTapDelay::AddTap(int32 TapId)
{
	FTapDelayInfo Info = FTapDelayInfo();
	Info.TapId = TapId;
	
	
	TargetTaps.Add(Info);

	int32 TapIndex = CurrentTaps.Add(FTapDelayInterpolationInfo());
	CurrentTaps[TapIndex].Init(SampleRate);

	bSettingsModified = true;
}

void FSubmixEffectTapDelay::RemoveTap(int32 TapId)
{
	for (int32 TapIndex = 0; TapIndex < TargetTaps.Num(); TapIndex++)
	{
		if (TargetTaps[TapIndex].TapId == TapId)
		{
			TargetTaps.RemoveAtSwap(TapIndex, 1, true);
			CurrentTaps.RemoveAtSwap(TapIndex, 1, true);
			bSettingsModified = true;
		}
	}
}

void FSubmixEffectTapDelay::SetTap(int32 TapId, const FTapDelayInfo& DelayInfo)
{
	for (FTapDelayInfo& TapInfo : TargetTaps)
	{
		if (TapInfo.TapId == TapId)
		{
			TapInfo.TapLineMode = DelayInfo.TapLineMode;
			TapInfo.DelayLength = FMath::Clamp(DelayInfo.DelayLength, 0.1f, MaxDelayLineLength);
			TapInfo.PanInDegrees = DelayInfo.PanInDegrees;
			TapInfo.OutputChannel = DelayInfo.OutputChannel;

			// cache the tap's gain as a linear value:
			TapInfo.Gain = Audio::ConvertToLinear(TapInfo.Gain);
			bSettingsModified = true;
			break;
		}
	}
}

void FSubmixEffectTapDelay::SetInterpolationTime(float Time)
{
	InterpolationTime = Time / 1000.0f;
	bSettingsModified = true;
}

void FSubmixEffectTapDelay::UpdateParameters()
{
	FSubmixEffectTapDelaySettings NewSettings;

	if (Params.GetParams(&NewSettings))
	{
		// If we have a new max delay line length, reallocate the delay line
		if (MaxDelayLineLength != NewSettings.MaximumDelayLength)
		{
			DelayLine.Reset();
			DelayLine.Init(SampleRate, NewSettings.MaximumDelayLength / 1000.0f);
			MaxDelayLineLength = NewSettings.MaximumDelayLength;
		}

		InterpolationTime = NewSettings.InterpolationTime / 1000.0f;

		// If we're being fed an empty list of taps, short circuit
		if (NewSettings.Taps.Num() == 0)
		{
			TargetTaps.Reset();
			CurrentTaps.Reset();
			return;
		}

		TargetTaps = NewSettings.Taps;

		// Update the number of current taps based on the number of target taps:
		const int32 CurrentNumTaps = CurrentTaps.Num();
		const int32 InterpolationsToAdd = TargetTaps.Num() - CurrentNumTaps;
		CurrentTaps.SetNum(TargetTaps.Num());

		// If there are any new current taps, initialize them:
		if (InterpolationsToAdd > 0)
		{
			for (int32 TapIndex = CurrentNumTaps; TapIndex < CurrentTaps.Num(); TapIndex++)
			{
				CurrentTaps[TapIndex].Init(SampleRate);
			}
		}

		// Cache all gain values as linear:
		for (int32 TapIndex = 0; TapIndex < TargetTaps.Num(); TapIndex++)
		{
			TargetTaps[TapIndex].Gain = Audio::ConvertToLinear(TargetTaps[TapIndex].Gain);
		}

		bSettingsModified = true;
	}


	if (bSettingsModified)
	{
		UpdateInterpolations();
		bSettingsModified = false;
	}
}

void FSubmixEffectTapDelay::UpdateInterpolations()
{
	// for each tap, pre-calculate our interpolation values
	for (int32 TapIndex = 0; TapIndex < CurrentTaps.Num(); TapIndex++)
	{
		FTapDelayInfo& TargetTap = TargetTaps[TapIndex];
		FTapDelayInterpolationInfo& CurrentTap = CurrentTaps[TapIndex];

		if (TargetTap.TapLineMode == ETapLineMode::Disabled)
		{
			// Do not change the delay line length and set target gain to 0.
			TargetTap.Gain = 0.0f;
		}
		else
		{
			// Clamp the delay line length to the maximum we can read, and calculate how much we should interpolate each sample.
			const float ClampedTarget = FMath::Clamp(TargetTap.DelayLength, 0.1f, MaxDelayLineLength);
			CurrentTap.SetLengthValue(ClampedTarget, InterpolationTime);
		}

		CurrentTap.SetGainValue(TargetTap.Gain, InterpolationTime);
	}
}

void USubmixEffectTapDelayPreset::SetInterpolationTime(float Time)
{
	DynamicSettings.InterpolationTime = Time;
	// Dispatch the added tap to all effect instances:
	for (FSoundEffectBase* EffectBaseInstance : Instances)
	{
		FSubmixEffectTapDelay* TapDelay = (FSubmixEffectTapDelay*)EffectBaseInstance;
		EffectBaseInstance->EffectCommand([TapDelay, Time]()
		{
			TapDelay->SetInterpolationTime(Time);
		});
	}
}

void USubmixEffectTapDelayPreset::OnInit()
{
	// Copy the settings to our dynamic settings so we can modify
	DynamicSettings = Settings;
}

void USubmixEffectTapDelayPreset::SetSettings(const FSubmixEffectTapDelaySettings& InSettings)
{
	DynamicSettings = InSettings;
	UpdateSettings(InSettings);
}

void USubmixEffectTapDelayPreset::AddTap(int32& OutTapId)
{
	FTapDelayInfo TapInfo = FTapDelayInfo();

	OutTapId = TapInfo.TapId;

	DynamicSettings.Taps.Add(TapInfo);

	// Dispatch the added tap to all effect instances:
	for (FSoundEffectBase* EffectBaseInstance : Instances)
	{
		FSubmixEffectTapDelay* TapDelay = (FSubmixEffectTapDelay*)EffectBaseInstance;
		EffectBaseInstance->EffectCommand([TapDelay, OutTapId]()
		{
			TapDelay->AddTap(OutTapId);
		});
	}
}

void USubmixEffectTapDelayPreset::RemoveTap(int32 TapId)
{
	for (int32 TapIndex = 0; TapIndex < DynamicSettings.Taps.Num(); TapIndex++)
	{
		if (DynamicSettings.Taps[TapIndex].TapId == TapId)
		{
			DynamicSettings.Taps.RemoveAtSwap(TapIndex);

			// Dispatch the tap removal to all effect instances:
			for (FSoundEffectBase* EffectBaseInstance : Instances)
			{
				FSubmixEffectTapDelay* TapDelayInstance = (FSubmixEffectTapDelay*)EffectBaseInstance;
				EffectBaseInstance->EffectCommand([TapDelayInstance, TapId]()
				{
					TapDelayInstance->RemoveTap(TapId);
				});
			}

			return;
		}
	}

	// if we got here, we couldn't find the tap ID.
	UE_LOG(LogTapDelay, Warning, TEXT("Tried to remove Invalid Tap ID %d!"), TapId);
	
}

void USubmixEffectTapDelayPreset::SetTap(int32 TapId, const FTapDelayInfo& TapInfo)
{
	for (FTapDelayInfo& DelayInfo : DynamicSettings.Taps)
	{
		if (DelayInfo.TapId == TapId)
		{
			// Copy over the tap delay info
			DelayInfo = TapInfo;

			// Restore the tap id to what it was since it just got stomped
			DelayInfo.TapId = TapId;

			// Dispatch the new tap to all effect instances:
			for (FSoundEffectBase* EffectBaseInstance : Instances)
			{
				FSubmixEffectTapDelay* TapDelayInstance = (FSubmixEffectTapDelay*)EffectBaseInstance;
				EffectBaseInstance->EffectCommand([TapDelayInstance, TapId, TapInfo]()
				{
					TapDelayInstance->SetTap(TapId, TapInfo);
				});
			}

			return;
		}
	}

	// if we got here, we couldn't find the tap ID.
	UE_LOG(LogTapDelay, Warning, TEXT("Tried to set Invalid Tap ID %d!"), TapId);
}

void USubmixEffectTapDelayPreset::GetTap(int32 TapId, FTapDelayInfo& TapInfo)
{
	for (FTapDelayInfo& Tap : DynamicSettings.Taps)
	{
		if (Tap.TapId == TapId)
		{
			TapInfo = Tap;
			return;
		}
	}

	// if we got here, we couldn't find the tap ID.
	UE_LOG(LogTapDelay, Warning, TEXT("Tried to get Invalid Tap ID %d!"), TapId);
}

void USubmixEffectTapDelayPreset::GetTapIds(TArray<int32>& TapIds)
{
	for (FTapDelayInfo& DelayInfo : DynamicSettings.Taps)
	{
		TapIds.Add(DelayInfo.TapId);
	}
}

void FTapDelayInterpolationInfo::Init(float SampleRate)
{
	LengthParam.Init(SampleRate);
	GainParam.Init(SampleRate);
}

void FTapDelayInterpolationInfo::SetGainValue(float Value, float InterpolationTime)
{
	GainParam.SetValue(Value, InterpolationTime);
}

float FTapDelayInterpolationInfo::GetGainValue()
{
	return GainParam.GetValue();
}

void FTapDelayInterpolationInfo::SetLengthValue(float Value, float InterpolationTime)
{
	LengthParam.SetValue(Value, InterpolationTime);
}

float FTapDelayInterpolationInfo::GetLengthValue()
{
	return LengthParam.GetValue();
}

