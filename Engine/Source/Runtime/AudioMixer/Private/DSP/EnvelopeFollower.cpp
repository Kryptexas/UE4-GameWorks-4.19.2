// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "DSP/EnvelopeFollower.h"
#include "DSP/Dsp.h"

namespace Audio
{
	FEnvelopeFollower::FEnvelopeFollower()
		: EnvMode(EEnvelopeFollowerMode::Peak)
		, TimeConstant(-1.00239343f)
		, SampleRate(44100.0f)
		, AttackTimeMsec(0.0f)
		, AttackTimeSamples(0.0f)
		, ReleaseTimeMsec(0.0f)
		, ReleaseTimeSamples(0.0f)
		, CurrentEnvelopeValue(0.0f)
	{
	}

	FEnvelopeFollower::FEnvelopeFollower(const float InSampleRate, const float InAttackTimeMsec, const float InReleaseTimeMSec, const EEnvelopeFollowerMode InMode)
		: EnvMode(InMode)
		, TimeConstant(-1.00239343f)
		, SampleRate(InSampleRate)
		, AttackTimeMsec(InAttackTimeMsec)
		, AttackTimeSamples(0.0f)
		, ReleaseTimeMsec(InReleaseTimeMSec)
		, ReleaseTimeSamples(0.0f)
		, CurrentEnvelopeValue(0.0f)
	{
		Init(InSampleRate);
	}

	FEnvelopeFollower::~FEnvelopeFollower()
	{
	}

	void FEnvelopeFollower::Init(const float InSampleRate)
	{
		SampleRate = InSampleRate;

		// Set the attack and release times using the default values
		SetAttackTime(10.0f);
		SetReleaseTime(10.0f);
	}

	void FEnvelopeFollower::Reset()
	{
		CurrentEnvelopeValue = 0.0f;
	}

	void FEnvelopeFollower::SetAttackTime(const float InAttackTimeMsec)
	{
		if (AttackTimeMsec != InAttackTimeMsec)
		{
			AttackTimeMsec = InAttackTimeMsec;
			AttackTimeSamples = FMath::Exp(1000.0f * TimeConstant / (AttackTimeMsec * SampleRate));
		}
	}

	void FEnvelopeFollower::SetReleaseTime(const float InReleaseTimeMsec)
	{
		if (ReleaseTimeMsec != InReleaseTimeMsec)
		{
			ReleaseTimeMsec = InReleaseTimeMsec;
			ReleaseTimeSamples = FMath::Exp(1000.0f * TimeConstant / (InReleaseTimeMsec * SampleRate));
		}
	}

	void FEnvelopeFollower::SetMode(const EEnvelopeFollowerMode InMode)
	{
		EnvMode = InMode;
	}

	float FEnvelopeFollower::ProcessAudio(const float InAudioSample)
	{
		// Take the absolute value of the input sample
		float Sample = FMath::Abs(InAudioSample);

		// If we're not Peak detecting, then square the input
		if (EnvMode != EEnvelopeFollowerMode::Peak)
		{
			Sample = Sample * Sample;
		}

		float TimeSamples = (Sample > CurrentEnvelopeValue) ? AttackTimeSamples : ReleaseTimeSamples;
		float NewEnvelopeValue = TimeSamples * (CurrentEnvelopeValue - Sample) + Sample;;
		NewEnvelopeValue = Audio::UnderflowClamp(NewEnvelopeValue);
		NewEnvelopeValue = FMath::Clamp(NewEnvelopeValue, 0.0f, 1.0f);

		// Update and return the envelope value
		return CurrentEnvelopeValue = NewEnvelopeValue;
	}

	float FEnvelopeFollower::GetCurrentValue() const
	{
		return CurrentEnvelopeValue;
	}

}
