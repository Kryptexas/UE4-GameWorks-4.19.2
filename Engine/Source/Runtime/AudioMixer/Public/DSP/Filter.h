// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BiQuadFilter.h"

namespace Audio
{
	// Enumeration of filter types
	namespace EFilter
	{
		enum Type
		{
			Lowpass,
			Highpass,
			Bandpass,
			Notch,
			ParametricEQ,
		};
	}

	// Helper function to get bandwidth from Q
	float GetBandwidthFromQ(const float InQ);

	// Helper function get Q from bandwidth
	float GetQFromBandwidth(const float InBandwidth);

	// Filter class which wraps a biquad filter object.
	// Handles multi-channel audio to avoid calculating filter coefficients for multiple channels of audio.
	class FFilter
	{
	public:
		// Constructor
		FFilter();
		// Destructor
		virtual ~FFilter();

		// Initialize the filter
		void Init(const float InSampleRate, const int32 InNumChannels, const EFilter::Type InType, const float InCutoffFrequency, const float InBandwidth, const float InGain = 0.0f);

		// Resets the filter state
		void Reset();

		// Processes a single frame of audio
		void ProcessAudioFrame(const float* InAudio, float* OutAudio);

		// Sets all filter parameters with one function
		void SetParams(const EFilter::Type InFilterType, const float InCutoffFrequency, const float InBandwidth, const float InGainDB);

		// Sets the type of the filter to use
		void SetType(const EFilter::Type InType);

		// Sets the filter frequency
		void SetFrequency(const float InCutoffFrequency);

		// Sets the bandwidth (octaves) of the filter
		void SetBandwidth(const float InBandwidth);

		// Sets the gain of the filter in decibels
		void SetGainDB(const float InGainDB);

		// Sets whether or no this filter is enabled (if disabled audio is passed through)
		void SetEnabled(const bool bInEnabled);

	protected:

		// Function computes biquad coefficients based on current filter settings
		void CalculateBiquadCoefficients();

		// What kind of filter to use when computing coefficients
		EFilter::Type FilterType;

		// Biquad filter objects for each channel
		FBiquad* Biquad;

		// The sample rate of the filter
		float SampleRate;

		// Number of channels in the filter
		int32 NumChannels;

		// Current frequency of the filter
		float Frequency;

		// Current bandwidth/resonance of the filter
		float Bandwidth;

		// The gain of the filter in DB (for filters that use gain)
		float GainDB;

		// Whether or not this filter is enabled (will bypass otherwise)
		bool bEnabled;
	};
}
