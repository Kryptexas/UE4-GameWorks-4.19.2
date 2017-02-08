// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace Audio
{
	// Different modes for the envelope follower
	enum class EEnvelopeFollowerMode
	{
		MeanSquared,
		RootMeanSquared,
		Peak,
	};

	// A simple utility that returns a smoothed value given audio input using an RC circuit.
	// Used for following the envelope of an audio stream.
	class AUDIOMIXER_API FEnvelopeFollower
	{
	public:

		FEnvelopeFollower();
		FEnvelopeFollower(const float InSampleRate, const float InAttackTimeMsec, const float InReleaseTimeMSec, const EEnvelopeFollowerMode InMode);

		virtual ~FEnvelopeFollower();

		// Initialize the envelope follower
		void Init(const float InSampleRate);

		// Resets the state of the envelope follower
		void Reset();

		// Sets the envelope follower attack time (how fast the envelope responds to input)
		void SetAttackTime(const float InAttackTimeMsec);

		// Sets the envelope follower release time (how slow the envelope dampens from input)
		void SetReleaseTime(const float InReleaseTimeMsec);

		// Sets the output mode of the envelope follower
		void SetMode(const EEnvelopeFollowerMode InMode);

		// Processes the input audio stream and returns the envelope value.
		float ProcessAudio(const float InAudioSample);

		// Gets the current envelope value
		float GetCurrentValue() const;

	protected:
		EEnvelopeFollowerMode EnvMode;
		float TimeConstant;
		float SampleRate;
		float AttackTimeMsec;
		float AttackTimeSamples;
		float ReleaseTimeMsec;
		float ReleaseTimeSamples;
		float CurrentEnvelopeValue;
	};


}
