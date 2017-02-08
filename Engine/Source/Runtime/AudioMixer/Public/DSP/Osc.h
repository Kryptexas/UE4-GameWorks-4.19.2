// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DSP/Dsp.h"
#include "DSP/ModulationMatrix.h"
#include "DSP/Noise.h"

namespace Audio
{
	// Struct which wraps all factors which contribute to pitch of the oscillator
	struct FOscFrequencyMod
	{	
		// A factor which directly scales the frequency output (used for FM modulation)
		float Scale;
		
		// The modulated frequency value
		float Mod;

		// Exponential frequency modulation
		float Detune;
		
		// Pitch bend modulation (i.e. from mod wheel)
		float PitchBend;

		// Octave frequency modulation
		float Octave;

		// Semitones frequency modulation
		float Semitones;

		// cents frequency modulation
		float Cents;

		FOscFrequencyMod()
			: Scale(1.0f)
			, Mod(0.0f)
			, Detune(0.0f)
			, PitchBend(0.0f)
			, Octave(0.0f)
			, Semitones(0.0f)
			, Cents(0.0f)
		{}

		FOscFrequencyMod(const FOscFrequencyMod& Other)
			: Scale(Other.Scale)
			, Detune(Other.Detune)
			, PitchBend(Other.PitchBend)
			, Octave(Other.Octave)
			, Semitones(Other.Semitones)
			, Cents(Other.Cents)
		{}
	};

	// Oscillator base class
	class AUDIOMIXER_API IOscBase
	{
	public:
		IOscBase();
		virtual ~IOscBase();

		// Initializes the oscillator
		virtual void Init(const float InSampleRate, const int32 InVoiceId = 0, FModulationMatrix* InMatrix = nullptr, const int32 ModMatrixStage = 1);

		// Starts the oscillator
		virtual void Start() = 0;

		// Stops the oscillator
		virtual void Stop() = 0;

		// Generates the next sample of audio, optionally passes out the auxillary output (supported in some oscillators)
		virtual float Generate(float* AuxOutput = nullptr) = 0;

		// Updates oscillator state
		virtual void Update();

		// Resets the oscillator
		virtual void Reset();

		// Sets the gain of the oscillator
		void SetGain(const float InGain) { Gain = InGain; }

		// Sets the gain modulator of the oscillator
		void SetGainMod(const float InGainMod) { GainMod = InGainMod; }

		// Sets the base frequency of the oscillator
		void SetFrequency(const float InFreqBase);

		// Sets the base frequency of the oscillator from the midi note number
		void SetNote(const float InNote);

		// Returns the frequency of the oscillator
		float GetFrequency() const { return BaseFreq; }

		void SetCents(const float InCents);
		void SetOctave(const float InOctave);
		void SetSemitones(const float InSemiTone);
		void SetDetune(const float InDetune);
		void SetPitchBend(const float InPitchBend);
		void SetFreqScale(const float InFreqScale);

		// Sets the LFO pulse width for square-wave type oscillators
		void SetPulseWidth(const float InPulseWidth);

		// Returns whether or not this oscillator is playing
		bool IsPlaying() const { return bIsPlaying; }

		// Return patch destinations for various modulatable parameters
		FPatchDestination GetModDestFrequency() const { return ModFrequencyDest; }
		FPatchDestination GetModDestPulseWidth() const { return ModPulseWidthDest; }
		FPatchDestination GetModDestGain() const { return ModGainDest; }
		FPatchDestination GetModDestAdd() const { return ModAddDest; }
		FPatchDestination GetModDestScale() const { return ModScaleDest; }

	protected:

		// Updates the phase based on the phase increment
		FORCEINLINE void UpdatePhase()
		{
			Phase += PhaseInc;
		}

		// Returns true if there was a phase wrap this update
		FORCEINLINE bool WrapPhase()
		{
			if (PhaseInc > 0.0f && Phase >= 1.0f)
			{
				Phase = FMath::Fmod(Phase, 1.0f);
				return true;
			}
			else if (PhaseInc < 0.0f && Phase <= 0.0f)
			{
				Phase = FMath::Fmod(Phase, 1.0f) + 1.0f;
				return true;
			}
			return false;
		}

		// Returns the current phase of the oscillator
		FORCEINLINE float GetPhase() const
		{
			return Phase;
		}

		// Returns the quadrature phase, wrapped
		FORCEINLINE float GetQuadPhase() const
		{
			return FMath::Fmod(Phase + 0.25f, 1.0f);
		}

	protected:

		// The voice ID that this oscillator belongs to
		int32 VoiceId;

		// Sample rate of the oscillator
		float SampleRate;

		// Maximum frequency allowed
		float Nyquist;

		// The final frequency of the oscillator after computing all factors contributing to frequency
		float Freq;

		// The base frequency of the oscillator
		float BaseFreq;

		// Holds all frequency data
		FOscFrequencyMod FreqData;

		// Linear gain of the oscillator
		float Gain;

		// Linear gain modulation of the oscillator (used in amplitude modulation)
		float GainMod;

		// The current phase of oscillator (between 0.0 and 1.0)
		float Phase;

		// How much to increment the phase each update
		float PhaseInc;

		// Pulse width used in square LFOs
		float PulseWidthBase;

		// Pulse width modulator factor
		float PulseWidthMod;

		// The final pulse width
		float PulseWidth;

		// Modulation matrix to use for this oscillator
		FModulationMatrix* ModMatrix;

		FPatchDestination ModFrequencyDest;
		FPatchDestination ModPulseWidthDest;
		FPatchDestination ModGainDest;
		FPatchDestination ModScaleDest;
		FPatchDestination ModAddDest;

		// Whether or not the oscillator is on or off
		bool bIsPlaying;
		bool bChanged;
	};

	namespace EOsc
	{
		enum Type
		{
			Sine,
			Saw,
			Triangle,
			Square,
			Noise,
			NumOscTypes
		};
	}

	// Pitched oscillator
	class AUDIOMIXER_API FOsc : public IOscBase
	{
	public:
		FOsc();
		virtual ~FOsc();

		//~ Begin FOscBase
		virtual void Start() override;
		virtual void Stop() override;
		virtual void Reset() override;
		virtual void Update() override;
		virtual float Generate(float* AuxOutput = nullptr) override;
		//~ End FOscBase

		// Sets the oscillator type
		void SetType(const EOsc::Type InType) { OscType = InType; }

		// Gets the oscillator type
		EOsc::Type GetType() const { return OscType; }

	protected:

		// Smooth out the saw-tooth discontinuities to improve aliasing
		float PolySmooth(const float InPhase, const float InPhaseInc);

		// Current sign of the square mod, used for triangle wave generation
		float TriangleSign; 
		
		// Used to store state for triangular differentiator
		float DPW_z1; 

		// The pulse width base lerped
		FExponentialEase PulseWidthLerped;

		// The type of the oscillator
		EOsc::Type OscType;

		// a noise generator
		FWhiteNoise Noise;
	};

}
