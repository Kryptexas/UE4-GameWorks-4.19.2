// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "DSP/Filter.h"
#include "DSP/Dsp.h"

namespace Audio
{
	float GetBandwidthFromQ(const float InQ)
	{
		// make sure Q is not 0.0f, clamp to slightly positive
		const float Q = FMath::Max(KINDA_SMALL_NUMBER, InQ);
		const float Arg = 0.5f * ((1.0f / Q) + FMath::Sqrt(1.0f / (Q*Q) + 4.0f));
		const float OutBandwidth = 2.0f * FMath::LogX(2.0f, Arg);
		return OutBandwidth;
	}

	float GetQFromBandwidth(const float InBandwidth)
	{
		const float InBandwidthClamped = FMath::Max(KINDA_SMALL_NUMBER, InBandwidth);
		const float Temp = FMath::Pow(2.0f, InBandwidthClamped);
		const float OutQ = FMath::Sqrt(Temp) / (Temp - 1.0f);
		return OutQ;
	}

	FFilter::FFilter()
		: FilterType(EFilter::Lowpass)
		, Biquad(nullptr)
		, SampleRate(0.0f)
		, NumChannels(0)
		, Frequency(0.0f)
		, Bandwidth(0.0f)
		, GainDB(0.0f)
		, bEnabled(true)
	{
	}

	FFilter::~FFilter()
	{
		if (Biquad)
		{
			delete[] Biquad;
		}
	}

	void FFilter::Init(const float InSampleRate, const int32 InNumChannels, const EFilter::Type InFilterType, const float InCutoffFrequency, const float InBandwidth, const float InGainDB)
	{
		SampleRate = InSampleRate;
		NumChannels = InNumChannels;
		FilterType = InFilterType;
		Frequency = FMath::Max(InCutoffFrequency, 20.0f);
		Bandwidth = InBandwidth;
		GainDB = InGainDB;

		if (Biquad)
		{
			delete[] Biquad;
		}

		Biquad = new FBiquad[NumChannels];
		Reset();
		CalculateBiquadCoefficients();
	}

	void FFilter::ProcessAudioFrame(const float* InAudio, float* OutAudio)
	{
		if (bEnabled)
		{
			for (int32 Channel = 0; Channel < NumChannels; ++Channel)
			{
				OutAudio[Channel] = Biquad[Channel].ProcessAudio(InAudio[Channel]);
			}
		}
		else
		{
			// Pass through if disabled
			for (int32 Channel = 0; Channel < NumChannels; ++Channel)
			{
				OutAudio[Channel] = InAudio[Channel];
			}
		}
	}

	void FFilter::Reset()
	{
		for (int32 Channel = 0; Channel < NumChannels; ++Channel)
		{
			Biquad[Channel].Reset();
		}
	}

	void FFilter::SetParams(const EFilter::Type InFilterType, const float InCutoffFrequency, const float InBandwidth, const float InGainDB)
	{
		const float InCutoffFrequencyClamped = FMath::Max(InCutoffFrequency, 20.0f);

		if (InFilterType != FilterType ||
			InCutoffFrequencyClamped != InCutoffFrequency ||
			Bandwidth != InBandwidth ||
			GainDB != InGainDB)
		{
			FilterType = InFilterType;
			Frequency = InCutoffFrequency;
			Bandwidth = InBandwidth;
			GainDB = InGainDB;
			CalculateBiquadCoefficients();
		}
	}

	void FFilter::SetType(const EFilter::Type InFilterType)
	{
		if (FilterType != InFilterType)
		{
			FilterType = InFilterType;
			CalculateBiquadCoefficients();
		}
	}

	void FFilter::SetFrequency(const float InCutoffFrequency)
	{
		const float InCutoffFrequencyClamped = FMath::Max(InCutoffFrequency, 20.0f);
		if (Frequency != InCutoffFrequencyClamped)
		{
			Frequency = InCutoffFrequencyClamped;
			CalculateBiquadCoefficients();
		}
	}

	void FFilter::SetBandwidth(const float InBandwidth)
	{
		if (Bandwidth != InBandwidth)
		{
			Bandwidth = InBandwidth;
			CalculateBiquadCoefficients();
		}
	}

	void FFilter::SetGainDB(const float InGainDB)
	{
		if (GainDB != InGainDB)
		{
			GainDB = InGainDB;
			if (FilterType == EFilter::ParametricEQ)
			{
				CalculateBiquadCoefficients();
			}
		}
	}

	void FFilter::SetEnabled(const bool bInEnabled)
	{
		bEnabled = bInEnabled;
	}

	void FFilter::CalculateBiquadCoefficients()
	{
		const float NaturalLog2 = 0.69314718055994530942f;

		const float Omega = 2.0f * PI * Frequency / SampleRate;
		const float Sn = FMath::Sin(Omega);
		const float Cs = FMath::Cos(Omega);

		const float Alpha = Sn * FMath::Sinh(0.5f * NaturalLog2 * Bandwidth * Omega / Sn);

		float a0;
		float a1;
		float a2;
		float b0;
		float b1;
		float b2;

		switch (FilterType)
		{
		default:
		case EFilter::Lowpass:
		{
			a0 = (1.0f - Cs) / 2.0f;
			a1 = (1.0f - Cs);
			a2 = (1.0f - Cs) / 2.0f;
			b0 = 1.0f + Alpha;
			b1 = -2.0f * Cs;
			b2 = 1.0f - Alpha;
		}
		break;

		case EFilter::Highpass:
		{
			a0 = (1.0f + Cs) / 2.0f;
			a1 = -(1.0f + Cs);
			a2 = (1.0f + Cs) / 2.0f;
			b0 = 1.0f + Alpha;
			b1 = -2.0f * Cs;
			b2 = 1.0f - Alpha;
		}
		break;

		case EFilter::Bandpass:
		{
			a0 = Alpha;
			a1 = 0.0f;
			a2 = -Alpha;
			b0 = 1.0f + Alpha;
			b1 = -2.0f * Cs;
			b2 = 1.0f - Alpha;
		}
		break;

		case EFilter::Notch:
		{
			a0 = 1.0f;
			a1 = -2.0f * Cs;
			a2 = 1.0f - Alpha;
			b0 = 1.0f + Alpha;
			b1 = -2.0f * Cs;
			b2 = 1.0f - Alpha;
		}
		break;

		case EFilter::ParametricEQ:
		{
			const float Amp = FMath::Pow(10.0f, GainDB / 40.0f);
			const float Beta = FMath::Sqrt(2.0f * Amp);

			a0 = 1.0f + (Alpha * Amp);
			a1 = -2.0f * Cs;
			a2 = 1.0f - (Alpha * Amp);
			b0 = 1.0f + (Alpha / Amp);
			b1 = -2.0f * Cs;
			b2 = 1.0f - (Alpha / Amp);
		}
		break;
		}

		a0 /= b0;
		a1 /= b0;
		a2 /= b0;
		b1 /= b0;
		b2 /= b0;

		for (int32 Channel = 0; Channel < NumChannels; ++Channel)
		{
			Biquad[Channel].A0 = a0;
			Biquad[Channel].A1 = a1;
			Biquad[Channel].A2 = a2;
			Biquad[Channel].B1 = b1;
			Biquad[Channel].B2 = b2;
		}
	}

}
