// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DSP/Delay.h"
#include "DSP/LFO.h"

namespace Audio
{
	namespace EChorusDelays
	{
		enum Type
		{
			Left,
			Center,
			Right,
			NumDelayTypes
		};
	}

	class AUDIOMIXER_API FChorus
	{
	public:
		FChorus();
		~FChorus();

		void Init(const float InSampleRate, const float InDelayLengthSec = 2.0f);

		void SetDepth(const EChorusDelays::Type InType, const float InDepth);
		void SetFrequency(const EChorusDelays::Type InType, const float InFrequency);
		void SetFeedback(const EChorusDelays::Type InType, const float InFeedback);
		void SetWetLevel(const float InWetLevel);
		void SetDelayMsecRange(const float InMinDelayMsec, const float InMaxDelayMsec);
		void ProcessAudio(const float InLeft, const float InRight, float& OutLeft, float& OutRight);

	protected:
		FDelay Delays[EChorusDelays::NumDelayTypes];
		FLFO LFOs[EChorusDelays::NumDelayTypes];
		float Depth[EChorusDelays::NumDelayTypes];
		float Feedback[EChorusDelays::NumDelayTypes];

		float MinDelayMsec;
		float MaxDelayMsec;
		float WetLevel;

		int32 ControlSampleCount;
		int32 ControlSamplePeriod;
	};


}
