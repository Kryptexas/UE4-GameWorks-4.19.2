// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "SynthComponents/EpicSynth1Component.h"


UEpicSynth1Component::UEpicSynth1Component(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EpicSynth1.Init(AUDIO_SAMPLE_RATE, 8);

	// Set default parameters
	SetEnablePolyphony(false);
	SetOscType(0, ESynth1OscType::Saw);
	SetOscType(1, ESynth1OscType::Saw);
	SetOscCents(1, 2.5f);
	SetOscPulsewidth(0, 0.5f);
	SetOscPulsewidth(1, 0.5f);
	SetEnableUnison(false);
	SetSpread(0.5f);
	SetGainDb(-3.0f);
	SetAttackTime(10.0f);
	SetDecayTime(100.0f);
	SetSustainGain(0.707f);
	SetReleaseTime(5000.0f);
	SetEnableLegato(true);
	SetEnableRetrigger(false);
	SetFilterFrequency(1200.0f);
	SetFilterQ(2.0f);
	SetFilterAlgorithm(ESynthFilterAlgorithm::Ladder);
	SetStereoDelayIsEnabled(true);
	SetStereoDelayMode(ESynthStereoDelayMode::PingPong);
	SetStereoDelayRatio(0.2f);
	SetStereoDelayFeedback(0.7f);
	SetStereoDelayWetlevel(0.3f);
	SetChorusEnabled(false);
}

void UEpicSynth1Component::NoteOn(const float Note, const int32 Velocity, const float Duration)
{
	SynthCommand([this, Note, Velocity, Duration]()
	{
		EpicSynth1.NoteOn((uint32)Note, (int32)Velocity, Duration);
	});
}

void UEpicSynth1Component::NoteOff(const float Note, const bool bAllNotesOff)
{
	SynthCommand([this, Note, bAllNotesOff]()
	{
		EpicSynth1.NoteOff((int32)Note, bAllNotesOff);
	});
}

void UEpicSynth1Component::SetEnablePolyphony(bool bEnablePolyphony)
{
	SynthCommand([this, bEnablePolyphony]()
	{
		EpicSynth1.SetMonoMode(!bEnablePolyphony);
	});
}

void UEpicSynth1Component::SetOscType(int32 OscIndex, ESynth1OscType OscType)
{
	SynthCommand([this, OscIndex, OscType]()
	{
		EpicSynth1.SetOscType(OscIndex, (Audio::EOsc::Type)OscType);
	});
}

void UEpicSynth1Component::SetOscOctave(int32 OscIndex, float Octave)
{
	SynthCommand([this, OscIndex, Octave]()
	{
		EpicSynth1.SetOscOctave(OscIndex, Octave);
	});
}

void UEpicSynth1Component::SetOscSemitones(int32 OscIndex, float Semitones)
{
	SynthCommand([this, OscIndex, Semitones]()
	{
		EpicSynth1.SetOscSemitones(OscIndex, Semitones);
	});
}

void UEpicSynth1Component::SetOscCents(int32 OscIndex, float Cents)
{
	SynthCommand([this, OscIndex, Cents]()
	{
		EpicSynth1.SetOscCents(OscIndex, Cents);
	});
}

void UEpicSynth1Component::SetPitchBend(float PitchBend)
{
	SynthCommand([this, PitchBend]()
	{
		EpicSynth1.SetOscPitchBend(0, PitchBend);
		EpicSynth1.SetOscPitchBend(1, PitchBend);
	});
}

void UEpicSynth1Component::SetPortamento(float Portamento)
{
	SynthCommand([this, Portamento]()
	{
		EpicSynth1.SetOscPortamento(Portamento);
	});
}

void UEpicSynth1Component::SetOscPulsewidth(int32 OscIndex, float Pulsewidth)
{
	SynthCommand([this, OscIndex, Pulsewidth]()
	{
		EpicSynth1.SetOscPulseWidth(OscIndex, Pulsewidth);
	});
}

void UEpicSynth1Component::SetEnableUnison(bool EnableUnison)
{
	SynthCommand([this, EnableUnison]()
	{
		EpicSynth1.SetOscUnison(EnableUnison);
	});
}

void UEpicSynth1Component::SetPan(float Pan)
{
	SynthCommand([this, Pan]()
	{
		EpicSynth1.SetPan(Pan);
	});
}

void UEpicSynth1Component::SetSpread(float Spread)
{
	SynthCommand([this, Spread]()
	{
		EpicSynth1.SetOscSpread(Spread);
	});
}

void UEpicSynth1Component::SetLFOFrequency(int32 LFOIndex, float FrequencyHz) 
{
	SynthCommand([this, LFOIndex, FrequencyHz]()
	{
		EpicSynth1.SetLFOFrequency(LFOIndex, FrequencyHz);
	});
}

void UEpicSynth1Component::SetLFOGain(int32 LFOIndex, float Gain) 
{
	SynthCommand([this, LFOIndex, Gain]()
	{
		EpicSynth1.SetLFOGain(LFOIndex, Gain);
	});
}

void UEpicSynth1Component::SetLFOType(int32 LFOIndex, ESynthLFOType LFOType)
{
	SynthCommand([this, LFOIndex, LFOType]()
	{
		EpicSynth1.SetLFOType(LFOIndex, (Audio::ELFO::Type)LFOType);
	});
}

void UEpicSynth1Component::SetLFOMode(int32 LFOIndex, ESynthLFOMode LFOMode)
{
	SynthCommand([this, LFOIndex, LFOMode]()
	{
		EpicSynth1.SetLFOMode(LFOIndex, (Audio::ELFOMode::Type)LFOMode);
	});
}

void UEpicSynth1Component::SetLFOPatch(int32 LFOIndex, ESynthLFOPatchType LFOPatchType)
{
	SynthCommand([this, LFOIndex, LFOPatchType]()
	{
		EpicSynth1.SetLFOPatch(LFOIndex, LFOPatchType);
	});
}

void UEpicSynth1Component::SetGainDb(float GainDb)
{
	SynthCommand([this, GainDb]()
	{
		EpicSynth1.SetGainDb(GainDb);
	});
}

void UEpicSynth1Component::SetAttackTime(float AttackTimeMsec)
{
	SynthCommand([this, AttackTimeMsec]()
	{
		EpicSynth1.SetEnvAttackTimeMsec(AttackTimeMsec);
	});
}

void UEpicSynth1Component::SetDecayTime(float DecayTimeMsec)
{
	SynthCommand([this, DecayTimeMsec]()
	{
		EpicSynth1.SetEnvDecayTimeMsec(DecayTimeMsec);
	});
}

void UEpicSynth1Component::SetSustainGain(float SustainGain)
{
	SynthCommand([this, SustainGain]()
	{
		EpicSynth1.SetEnvSustainGain(SustainGain);
	});
}

void UEpicSynth1Component::SetReleaseTime(float ReleaseTimeMsec)
{
	SynthCommand([this, ReleaseTimeMsec]()
	{
		EpicSynth1.SetEnvReleaseTimeMsec(ReleaseTimeMsec);
	});
}

void UEpicSynth1Component::SetModEnvPatch(const ESynthEnvModPatch InPatchType)
{
	SynthCommand([this, InPatchType]()
	{
		EpicSynth1.SetModEnvPatch(InPatchType);
	});
}

void UEpicSynth1Component::SetModEnvInvert(const bool bInInvert)
{
	SynthCommand([this, bInInvert]()
	{
		EpicSynth1.SetModEnvInvert(bInInvert);
	});
}

void UEpicSynth1Component::SetEnableLegato(bool LegatoEnabled)
{
	SynthCommand([this, LegatoEnabled]()
	{
		EpicSynth1.SetEnvLegatoEnabled(LegatoEnabled);
	});
}

void UEpicSynth1Component::SetEnableRetrigger(bool RetriggerEnabled)
{
	SynthCommand([this, RetriggerEnabled]()
	{
		EpicSynth1.SetEnvRetriggerMode(RetriggerEnabled);
	});
}

void UEpicSynth1Component::SetFilterFrequency(float FilterFrequencyHz)
{
	SynthCommand([this, FilterFrequencyHz]()
	{
		EpicSynth1.SetFilterFrequency(FilterFrequencyHz);
	});
}

void UEpicSynth1Component::SetFilterQ(float FilterQ)
{
	SynthCommand([this, FilterQ]()
	{
		EpicSynth1.SetFilterQ(FilterQ);
	});
}

void UEpicSynth1Component::SetFilterType(ESynthFilterType FilterType)
{
	SynthCommand([this, FilterType]()
	{
		EpicSynth1.SetFilterType((Audio::EFilter::Type)FilterType);
	});
}

void UEpicSynth1Component::SetFilterAlgorithm(ESynthFilterAlgorithm FilterAlgorithm)
{
	SynthCommand([this, FilterAlgorithm]()
	{
		EpicSynth1.SetFilterAlgorithm(FilterAlgorithm);
	});
}

void UEpicSynth1Component::SetStereoDelayIsEnabled(bool StereoDelayEnabled)
{
	SynthCommand([this, StereoDelayEnabled]()
	{
		EpicSynth1.SetStereoDelayIsEnabled(StereoDelayEnabled);
	});
}

void UEpicSynth1Component::SetStereoDelayMode(ESynthStereoDelayMode StereoDelayMode)
{
	SynthCommand([this, StereoDelayMode]()
	{
		EpicSynth1.SetStereoDelayMode((Audio::EStereoDelayMode::Type)StereoDelayMode);
	});
}

void UEpicSynth1Component::SetStereoDelayTime(float DelayTimeMsec)
{
	SynthCommand([this, DelayTimeMsec]()
	{
		EpicSynth1.SetStereoDelayTimeMsec(DelayTimeMsec);
	});
}

void UEpicSynth1Component::SetStereoDelayFeedback(float DelayFeedback)
{
	SynthCommand([this, DelayFeedback]()
	{
		EpicSynth1.SetStereoDelayFeedback(DelayFeedback);
	});
}

void UEpicSynth1Component::SetStereoDelayWetlevel(float DelayWetlevel)
{
	SynthCommand([this, DelayWetlevel]()
	{
		EpicSynth1.SetStereoDelayWetLevel(DelayWetlevel);
	});
}

void UEpicSynth1Component::SetStereoDelayRatio(float DelayRatio)
{
	SynthCommand([this, DelayRatio]()
	{
		EpicSynth1.SetStereoDelayRatio(DelayRatio);
	});
}

void UEpicSynth1Component::SetChorusEnabled(bool EnableChorus)
{
	SynthCommand([this, EnableChorus]()
	{
		EpicSynth1.SetChorusEnabled(EnableChorus);
	});
}

void UEpicSynth1Component::SetChorusDepth(float Depth)
{
	SynthCommand([this, Depth]()
	{
		EpicSynth1.SetChorusDepth(Audio::EChorusDelays::Left, Depth);
		EpicSynth1.SetChorusDepth(Audio::EChorusDelays::Center, Depth);
		EpicSynth1.SetChorusDepth(Audio::EChorusDelays::Right, Depth);
	});
}

void UEpicSynth1Component::SetChorusFeedback(float Feedback)
{
	SynthCommand([this, Feedback]()
	{
		EpicSynth1.SetChorusFeedback(Audio::EChorusDelays::Left, Feedback);
		EpicSynth1.SetChorusFeedback(Audio::EChorusDelays::Center, Feedback);
		EpicSynth1.SetChorusFeedback(Audio::EChorusDelays::Right, Feedback);
	});
}

void UEpicSynth1Component::SetChorusFrequency(float Frequency) 
{
	SynthCommand([this, Frequency]()
	{
		EpicSynth1.SetChorusFrequency(Audio::EChorusDelays::Left, Frequency);
		EpicSynth1.SetChorusFrequency(Audio::EChorusDelays::Center, Frequency);
		EpicSynth1.SetChorusFrequency(Audio::EChorusDelays::Right, Frequency);
	});
}

void UEpicSynth1Component::SetSynthPreset(const FEpicSynth1Preset& SynthPreset)
{
	SynthCommand([this, SynthPreset]()
	{
		EpicSynth1.SetMonoMode(!SynthPreset.bEnablePolyphony);

		EpicSynth1.SetOscType(0, (Audio::EOsc::Type)SynthPreset.Osc1Type);
		EpicSynth1.SetOscOctave(0, SynthPreset.Osc1Octave);
		EpicSynth1.SetOscSemitones(0, SynthPreset.Osc1Semitones);
		EpicSynth1.SetOscCents(0, SynthPreset.Osc1Cents);
		EpicSynth1.SetOscPulseWidth(0, SynthPreset.Osc1PulseWidth);

		EpicSynth1.SetOscType(1, (Audio::EOsc::Type)SynthPreset.Osc2Type);
		EpicSynth1.SetOscOctave(1, SynthPreset.Osc2Octave);
		EpicSynth1.SetOscSemitones(1, SynthPreset.Osc2Semitones);
		EpicSynth1.SetOscCents(1, SynthPreset.Osc2Cents);
		EpicSynth1.SetOscPulseWidth(1, SynthPreset.Osc2PulseWidth);

		EpicSynth1.SetOscPortamento(SynthPreset.Portamento);
		EpicSynth1.SetOscUnison(SynthPreset.bEnableUnison);

		EpicSynth1.SetOscSpread(SynthPreset.Spread);
		EpicSynth1.SetPan(SynthPreset.Pan);

		EpicSynth1.SetLFOFrequency(0, SynthPreset.LFO1Frequency);
		EpicSynth1.SetLFOGain(0, SynthPreset.LFO1Gain);
		EpicSynth1.SetLFOType(0, (Audio::ELFO::Type)SynthPreset.LFO1Type);
		EpicSynth1.SetLFOMode(0, (Audio::ELFOMode::Type)SynthPreset.LFO1Mode);
		EpicSynth1.SetLFOPatch(0, SynthPreset.LFO1PatchType);

		EpicSynth1.SetLFOFrequency(1, SynthPreset.LFO2Frequency);
		EpicSynth1.SetLFOGain(1, SynthPreset.LFO2Gain);
		EpicSynth1.SetLFOType(1, (Audio::ELFO::Type)SynthPreset.LFO2Type);
		EpicSynth1.SetLFOMode(1, (Audio::ELFOMode::Type)SynthPreset.LFO2Mode);
		EpicSynth1.SetLFOPatch(1, SynthPreset.LFO2PatchType);

		EpicSynth1.SetGainDb(SynthPreset.GainDb);
		
		EpicSynth1.SetEnvAttackTimeMsec(SynthPreset.AttackTime);
		EpicSynth1.SetEnvDecayTimeMsec(SynthPreset.DecayTime);
		EpicSynth1.SetEnvSustainGain(SynthPreset.SustainGain);
		EpicSynth1.SetEnvReleaseTimeMsec(SynthPreset.ReleaseTime);

		EpicSynth1.SetModEnvPatch(SynthPreset.EnvModPatchType);
		EpicSynth1.SetModEnvInvert(SynthPreset.bInvertModEnv);

		EpicSynth1.SetEnvLegatoEnabled(SynthPreset.bLegato);
		EpicSynth1.SetEnvRetriggerMode(SynthPreset.bRetrigger);

		EpicSynth1.SetFilterFrequency(SynthPreset.FilterFrequency);
		EpicSynth1.SetFilterQ(SynthPreset.FilterQ);
		EpicSynth1.SetFilterType((Audio::EFilter::Type)SynthPreset.FilterType);
		EpicSynth1.SetFilterAlgorithm(SynthPreset.FilterAlgorithm);

		EpicSynth1.SetStereoDelayIsEnabled(SynthPreset.bStereoDelayEnabled);
		EpicSynth1.SetStereoDelayMode((Audio::EStereoDelayMode::Type)SynthPreset.StereoDelayMode);
		EpicSynth1.SetStereoDelayTimeMsec(SynthPreset.StereoDelayTime);
		EpicSynth1.SetStereoDelayFeedback(SynthPreset.StereoDelayFeedback);
		EpicSynth1.SetStereoDelayWetLevel(SynthPreset.StereoDelayWetlevel);
		EpicSynth1.SetStereoDelayRatio(SynthPreset.StereoDelayRatio);

		EpicSynth1.SetChorusEnabled(SynthPreset.bChorusEnabled);
		EpicSynth1.SetChorusDepth(Audio::EChorusDelays::Left, SynthPreset.ChorusDepth);
		EpicSynth1.SetChorusDepth(Audio::EChorusDelays::Center, SynthPreset.ChorusDepth);
		EpicSynth1.SetChorusDepth(Audio::EChorusDelays::Right, SynthPreset.ChorusDepth);
		EpicSynth1.SetChorusFeedback(Audio::EChorusDelays::Left, SynthPreset.ChorusFeedback);
		EpicSynth1.SetChorusFeedback(Audio::EChorusDelays::Center, SynthPreset.ChorusFeedback);
		EpicSynth1.SetChorusFeedback(Audio::EChorusDelays::Right, SynthPreset.ChorusFeedback);
		EpicSynth1.SetChorusFrequency(Audio::EChorusDelays::Left, SynthPreset.ChorusFrequency);
		EpicSynth1.SetChorusFrequency(Audio::EChorusDelays::Center, SynthPreset.ChorusFrequency);
		EpicSynth1.SetChorusFrequency(Audio::EChorusDelays::Right, SynthPreset.ChorusFrequency);

		// Now loop through all the patches
		for (const FEpicSynth1Patch& Patch : SynthPreset.Patches)
		{
			if (Patch.PatchCables.Num() > 0)
			{
				EpicSynth1.CreatePatch(Patch.PatchSource, Patch.PatchCables, true);
			}
		}

	});
}

FPatchId UEpicSynth1Component::CreatePatch(const ESynth1PatchSource PatchSource, const TArray<FSynth1PatchCable>& PatchCables, const bool bEnabledByDefault)
{
	return EpicSynth1.CreatePatch(PatchSource, PatchCables, bEnabledByDefault);
}

bool UEpicSynth1Component::SetEnablePatch(const FPatchId PatchId, const bool bIsEnabled)
{
	return EpicSynth1.SetEnablePatch(PatchId, bIsEnabled);
}


void UEpicSynth1Component::OnGenerateAudio(TArray<float>& OutAudio)
{
	const int32 NumFrames = OutAudio.Num() / NumChannels;

	float LeftSample = 0.0f;
	float RightSample = 0.0f;
	int32 SampleIndex = 0;

	for (int32 Frame = 0; Frame < NumFrames; ++Frame)
	{
		EpicSynth1.Generate(LeftSample, RightSample);

		OutAudio[SampleIndex++] = LeftSample;
		OutAudio[SampleIndex++] = RightSample;
	}

}
