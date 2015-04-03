// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "UnrealAudioModule.h"

#if ENABLE_UNREAL_AUDIO

// We need to define TWO_PI for our FM synthesizer
#ifndef TWO_PI
#define TWO_PI (6.28318530718)
#endif

#ifndef PI_OVER_TWO
#define PI_OVER_TWO (1.57079632679)
#endif

namespace UAudio
{
namespace Test
{
	/**
	* FTimer
	* A simple timer that tracks a timer event based on current absolute input time.
	*/
	class FTimer
	{
	public:
		FTimer(double TotalTime);
		~FTimer();

		bool Update();
		bool IsDone() const;
		void Start(double InTotalTime);
		void Reset();
		float GetTotalTime() const;

	private:
		double TotalTime;
		double StartTime;
		double CurrentTime;
		double LastTime;
	};

	/**
	* FFader
	* A simple sample-accurate fader
	*/
	class FFader
	{
	private:
		struct EFadeState
		{
			enum Type
			{
				FADE_IN,
				FADE_OUT,
				OFF,
				ON
			};
		};

	public:
		FFader(float FadeInEase, float FadeOutEase);
		~FFader();

		float Update();
		void Fade(bool bFadeIn);
		void Toggle();

	private:
		float FadeInEase;
		float FadeOutEase;
		float FadeValue;
		EFadeState::Type FadeState;
	};

	/**
	* FBiquad
	* A simple biquad filter implementation.
	*/
	class FBiquad
	{
	public:
		FBiquad();
		~FBiquad();

		float Update(float Value);

	protected:
		float X1; // X(n) * Z^-1
		float X2; // X(n) * Z^-2
		float Y1; // Y(n) * Z^-1
		float Y2; // Y(n) * Z^-2

		float A0; // input coefficients
		float A1;
		float A2;
		float B1; // output coefficients
		float B2;
	};

	/**
	* FLowPass
	* Simple 2-pole lowpass filter using a biquad filter as a base class
	*/
	class FLowPass : public FBiquad
	{
	public:
		FLowPass();
		~FLowPass();

		void SetFrequency(float InFrequency);
		void SetQuality(float InQuality);
		void SetParams(float InFrequency, float InQuality);

	private:
		float Quality;
		float Frequency;
	};

	/**
	* FRamp
	* A linear time-ramp class.
	*/
	class FRamp
	{
	public:
		FRamp();
		~FRamp();

		void SetFrequency(float InFrequency);
		void SetScaleAdd(float InScale, float InAdd);
		float Update();

	private:
		float CurrValue;
		float DeltaValue;
		float Frequency;
		float Scale;
		float Add;
	};

	/**
	* FSineOsc
	* Simple sinusoidal oscillator class.
	*/
	class FSineOsc
	{
	public:
		FSineOsc();
		~FSineOsc();

		void SetFrequency(float InFrequency);
		void SetScaleAdd(float InScale, float InAdd);
		void SetOutputRange(float Min, float Max);

		float Update();

	private:
		float Frequency;
		float Phase;
		float PhaseDelta;
		float PhaseDeltaDelta;
		float TargetPhaseDelta;
		float Scale;
		float Add;
		bool bNewValue;
	};

	/**
	* FPan
	* Object which takes a normalized pan position and performs spatialization based on output speaker mappings.
	*/
	class FPan
	{
	public:
		FPan();
		~FPan();

		void Init(float InPan);
		void SetPan(float InPan);
		void Spatialize(float Value, float* Frame);

	private:
		int32 GetOutputSpeaker(int32 MapIndex);

		float Pan;
		int32 LFEIndex;
		int32 PrevSpeakerIndex;
		int32 NumNonLfeSpeakers;
		TArray<float> SpeakerMap;

		static int32 StereoSpeakerIndexMap[];
		static int32 QuadSpeakerIndexMap[];
		static int32 FiveOneSpeakerIndexMap[];
		static int32 SevenOneSpeakerIndexMap[];
	};

	/**
	* FDelay
	* A single line, multi-tap delay object. Each tap can be set with different params.
	*/
	class FDelay
	{
	public:
		FDelay();
		~FDelay();

		void Init(int32 NumTaps, float InMaxLengthSec);
		void SetDelayLength(int32 Tap, float LengthSec);
		void SetWet(int32 Tap, float WetLevel);
		void SetFeedback(int32 Tap, float FeedbackLevel);
		void GetOutput(float InSample, TArray<float>& TapOutput);

	private:
		struct FTap
		{
			float DelayFrames;
			int32 ReadIndex;
			float Wet;
			float Feedback;

			FTap()
				: DelayFrames(0.0f)
				, ReadIndex(0)
				, Wet(0.5f)
				, Feedback(0.5f)
			{
			}
		};

		float MaxLengthSec;
		int32 LengthFrames;
		int32 WriteIndex;
		TArray<FTap> Taps;
		TArray<float> DelayBuffer;
	};


	/**
	* IGenerator
	* An interface for classes which generates audio with give input/output audio buffers.
	*/
	class IGenerator
	{
	public:
		IGenerator(double LifeTime);
		virtual ~IGenerator() {}

		virtual bool GetNextBuffer(FCallbackInfo& CallbackInfo) = 0;
		bool IsDone() const;

	protected:
		bool UpdateTimers();

		FTimer LifeTimer;
		bool bIsDone;
		bool bIsInit;
	};

	/**
	* FSimpleOutput
	* Class which plays sinusoidal decays in increasing/decreasing harmonics in each speaker at different rates.
	*/
	class FSimpleOutput : public IGenerator
	{
	public:
		FSimpleOutput(double LifeTime);
		~FSimpleOutput();

		bool GetNextBuffer(FCallbackInfo& CallbackInfo) override;

	private:
		bool IsInit();

		struct FSpeakerInfo
		{
			float Phase;
			FFader Fader;
			int32 Harmonic;
			int32 MaxHarmonic;
			FTimer Timer;
			float Time;
			bool bDirection;
			bool bOn;

			FSpeakerInfo()
				: Phase(0.0f)
				, Fader(0.01f, 0.0001f)
				, Harmonic(1)
				, MaxHarmonic(10)
				, Timer(1.0f)
				, Time(1.0f)
				, bDirection(true)
				, bOn(false)
			{
			}
		};

		float BaseFreqHz;
		float PhaseDelta;
		float Amplitude;
		TArray<FSpeakerInfo> SpeakerInfo;
		int32 NumNonLfeSpeakers;
		int32 LFEIndex;
	};


	/**
	* FRandomizedFmGenerator
	* A bank of PM/FM synthesizers that have randomized and time-modulated parameters.
	*/
	class FPhaseModulator : public IGenerator
	{
	public:
		FPhaseModulator(double LifeTime);
		~FPhaseModulator();

		bool GetNextBuffer(FCallbackInfo& CallbackInfo) override;

	private:

		class OscData
		{
		public:
			OscData(float InMaxAmp);
			void Reset();
			void Update(bool bIsCarrier = false);
			float GetAmp() const;
			float GetValue() const;
			float GetPhase() const;

		private:
			float Phase;
			float Delta;
			float TargetDelta;
			float DeltaEase;
			float SweepPhase;
			float SweepDelta;
			float Amp;
			float MaxAmp;
			bool bOscilateSweep;
		};

		class FSynth
		{
		public:
			FSynth(FPhaseModulator* InParent);
			~FSynth();

			void GetNextFrame(float* Frame);

		private:
			void Init();

			FPhaseModulator* Parent;
			OscData Carrier;
			OscData ModIndex;
			int32 PrevSpeakerIndex;
			TArray<OscData> Mods;
			TArray<float> SpeakerMap;
			FLowPass LowPass;
			FPan Panner;
			FRamp PanRamp;
			FSineOsc FilterLFO;
			bool bInit;
		};

		friend class FSynth;

		float Amplitude;
		TArray<FSynth> SynthesisData;
		int32 TotalNumSynthesizers;
		int32 CurrNumSynthesisers;
	};

	/**
	* FPassThrough
	* A simple input pass-through to output (mic->speakers). Should hear a bit of delay from input to output due to device latency.
	*/
	class FPassThrough : public IGenerator
	{
	public:
		FPassThrough(double LifeTime);
		~FPassThrough();

		bool GetNextBuffer(FCallbackInfo& CallbackInfo) override;
	};

	/**
	* FSimpleRandomizedDelayer
	* An simple randomized delay line that delays input audio stream by various amount to great effect. 
	*/
	class FInputDelay : public IGenerator
	{
	public:
		FInputDelay(double LifeTime);
		~FInputDelay();

		bool GetNextBuffer(FCallbackInfo& CallbackInfo) override;

	private:
		void Init();

		bool bInit;
		FDelay Delay;
		TArray<float> DelayOutput;
	};

	/**
	* FNoisePan
	* An simple test which generates noise and pans it clockwise
	*/
	class FNoisePan : public IGenerator
	{
	public:
		FNoisePan(double LifeTime);
		~FNoisePan();

		bool GetNextBuffer(FCallbackInfo& CallbackInfo) override;

		float Amp;
		FLowPass LowPass;
		FPan Panner;
		FRamp PanRamp;
		FSineOsc FilterLFO;
		FDelay Delay;
		bool bInit;
	};

	/**
	* Updates any callback data to use with generators based on the callback info struct
	*/
	void UpdateCallbackData(FCallbackInfo& CallbackInfo);

} // namespace Test
} // namespace UAudio

#endif // #if ENABLE_UNREAL_AUDIO