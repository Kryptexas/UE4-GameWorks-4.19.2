// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "DSP/Chorus.h"

namespace Audio
{
	FChorus::FChorus()
		: MinDelayMsec(5.0f)
		, MaxDelayMsec(30.0f)
		, WetLevel(0.5f)
		, ControlSampleCount(0)
		, ControlSamplePeriod(256)
	{
		FMemory::Memzero(Depth, sizeof(float)*EChorusDelays::NumDelayTypes);
		FMemory::Memzero(Feedback, sizeof(float)*EChorusDelays::NumDelayTypes);
	}

	FChorus::~FChorus()
	{
	}

	void FChorus::Init(const float InSampleRate, const float InBufferLengthSec)
	{
		const float ControlSampleRate = InSampleRate / ControlSamplePeriod;
		for (int32 i = 0; i < EChorusDelays::NumDelayTypes; ++i)
		{
			Delays[i].Init(InSampleRate, InBufferLengthSec);
			
			LFOs[i].Init(ControlSampleRate);
			LFOs[i].SetType(ELFO::Triangle);
			LFOs[i].Update();
			LFOs[i].Start();
		}
	}

	void FChorus::SetDepth(const EChorusDelays::Type InType, const float InDepth)
	{
		Depth[InType] = FMath::Clamp(InDepth, 0.0f, 1.0f);
	}

	void FChorus::SetFrequency(const EChorusDelays::Type InType, const float InFrequency)
	{
		LFOs[InType].SetFrequency(InFrequency);
		LFOs[InType].Update();
	}

	void FChorus::SetFeedback(const EChorusDelays::Type InType, const float InFeedback)
	{
		Feedback[InType] = FMath::Clamp(InFeedback, 0.0f, 1.0f);
	}

	void FChorus::SetWetLevel(const float InWetLevel)
	{
		WetLevel = InWetLevel;
	}

	void FChorus::SetDelayMsecRange(const float InMinDelayMsec, const float InMaxDelayMsec)
	{
		MinDelayMsec = FMath::Max(InMinDelayMsec, 0.0f);
		MaxDelayMsec = FMath::Max(InMaxDelayMsec, MinDelayMsec + 1.0f);
	}

	void FChorus::ProcessAudio(const float InLeft, const float InRight, float& OutLeft, float& OutRight)
	{
		ControlSampleCount = ControlSampleCount & (ControlSamplePeriod - 1);
		if (ControlSampleCount == 0)
		{
			float LFONormalPhaseOut = 0.0f;
			float LFOQuadPhaseOut = 0.0f;
			for (int32 i = 0; i < EChorusDelays::NumDelayTypes; ++i)
			{
				LFONormalPhaseOut = LFOs[i].Generate(&LFOQuadPhaseOut);

				if (i == EChorusDelays::Left)
				{
					const float NewDelay = LFOQuadPhaseOut * Depth[i] * (MaxDelayMsec - MinDelayMsec) + MinDelayMsec;
					Delays[i].SetDelayMsec(NewDelay);
				}
				else if (i == EChorusDelays::Center)
				{
					const float NewDelay = LFONormalPhaseOut * Depth[i] * (MaxDelayMsec - MinDelayMsec) + MinDelayMsec;
					Delays[i].SetDelayMsec(NewDelay);
				}
				else
				{
					const float NewDelay = -LFOQuadPhaseOut * Depth[i] * (MaxDelayMsec - MinDelayMsec) + MinDelayMsec;
					Delays[i].SetDelayMsec(NewDelay);
				}
			}
		}
		++ControlSampleCount;

		float DelayInputs[EChorusDelays::NumDelayTypes];

		DelayInputs[EChorusDelays::Left] = InLeft;
		DelayInputs[EChorusDelays::Center] = 0.5f * InLeft + 0.5f * InRight;
		DelayInputs[EChorusDelays::Right] = InRight;

		float DelayOutputs[EChorusDelays::NumDelayTypes];

		for (int32 i = 0; i < EChorusDelays::NumDelayTypes; ++i)
		{
			DelayOutputs[i] = Delays[i].Read();

			Delays[i].WriteDelayAndInc(DelayInputs[i] + DelayOutputs[i] * Feedback[i]);
		}

		const float DryLevel = (1.0f - WetLevel);

		OutLeft = InLeft * DryLevel + WetLevel * (DelayOutputs[EChorusDelays::Left] + 0.5f * DelayOutputs[EChorusDelays::Center]);
		
		OutRight = InRight * DryLevel + WetLevel * (DelayOutputs[EChorusDelays::Right] + 0.5f * DelayOutputs[EChorusDelays::Center]);
	}
}
