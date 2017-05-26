// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DSP/Dsp.h"

namespace Audio
{
	// Simple 1-pole lowpass filter
	class AUDIOMIXER_API FOnePoleLPF
	{
	public:

		// Constructor 
		FOnePoleLPF()
			: CutoffFrequency(0.0f)
			, B1(0.0f)
			, A0(1.0f)
			, Z1(0.0f)
		{}

		// Set the LPF gain coefficient
		FORCEINLINE void SetG(float InG)
		{ 
			B1 = InG;
			A0 = 1.0f - B1;
		}

		// Resets the sample delay to 0
		void Reset()
		{
			B1 = 0.0f;
			A0 = 1.0f;
			Z1 = 0.0f;
		}

		/** Sets the filter frequency using normalized frequency (between 0.0 and 1.0f or 0.0 hz and Nyquist Frequency in Hz) */
		FORCEINLINE void SetFrequency(const float InFrequency)
		{
			CutoffFrequency = InFrequency;
			B1 = FMath::Exp(-PI * CutoffFrequency);
			A0 = 1.0f - B1;
		}

		float ProcessAudio(const float InputSample)
		{
			float Yn = InputSample*A0 + B1*Z1;
			Yn = UnderflowClamp(Yn);
			Z1 = Yn;
			return Yn;
		}

		// Process audio
		void ProcessAudio(const float* InputSample, float* OutputSample)
		{
			*OutputSample = ProcessAudio(*InputSample);
		}

	protected:
		float CutoffFrequency;

		// Filter coefficients
		float B1;
		float A0;

		// 1-sample delay
		float Z1;
	};

}
