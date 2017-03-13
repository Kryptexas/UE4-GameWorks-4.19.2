// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/AudioComponent.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "UObject/Class.h"
#include "Engine/EngineTypes.h"
#include "Sound/SoundWaveProcedural.h"
#include "Classes/Engine/DataTable.h"
#include "Components/SynthComponent.h"
#include "EpicSynth1.h"
#include "EpicSynth1Types.h"
#include "DSP/Osc.h"
#include "EpicSynth1Component.generated.h"

USTRUCT(BlueprintType)
struct SYNTHESIS_API FEpicSynth1Patch
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	ESynth1PatchSource PatchSource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	TArray<FSynth1PatchCable> PatchCables;
};

USTRUCT(BlueprintType)
struct SYNTHESIS_API FEpicSynth1Preset : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	uint32 bEnablePolyphony : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	ESynth1OscType Osc1Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float Osc1Octave;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float Osc1Semitones;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float Osc1Cents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float Osc1PulseWidth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	ESynth1OscType Osc2Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float Osc2Octave;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float Osc2Semitones;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float Osc2Cents;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float Osc2PulseWidth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float Portamento;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	uint32 bEnableUnison : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float Spread;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float Pan;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float LFO1Frequency;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float LFO1Gain;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	ESynthLFOType LFO1Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	ESynthLFOMode LFO1Mode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	ESynthLFOPatchType LFO1PatchType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float LFO2Frequency;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float LFO2Gain;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	ESynthLFOType LFO2Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	ESynthLFOMode LFO2Mode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	ESynthLFOPatchType LFO2PatchType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float GainDb;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float AttackTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float DecayTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float SustainGain;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float ReleaseTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	ESynthEnvModPatch EnvModPatchType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	uint32 bInvertModEnv : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	uint32 bLegato : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	uint32 bRetrigger : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float FilterFrequency;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float FilterQ;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	ESynthFilterType FilterType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	ESynthFilterAlgorithm FilterAlgorithm;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	uint32 bStereoDelayEnabled : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	ESynthStereoDelayMode StereoDelayMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float StereoDelayTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float StereoDelayFeedback;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float StereoDelayWetlevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float StereoDelayRatio;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	uint32 bChorusEnabled : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float ChorusDepth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float ChorusFeedback;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	float ChorusFrequency;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	TArray<FEpicSynth1Patch> Patches;

	FEpicSynth1Preset()
		: bEnablePolyphony(false)
		, Osc1Type(ESynth1OscType::Saw)
		, Osc1Octave(0.0f)
		, Osc1Semitones(0.0f)
		, Osc1Cents(0.0f)
		, Osc1PulseWidth(0.5f)
		, Osc2Type(ESynth1OscType::Saw)
		, Osc2Octave(0.0f)
		, Osc2Semitones(0.0f)
		, Osc2Cents(2.5f)
		, Osc2PulseWidth(0.5f)
		, Portamento(0.0f)
		, bEnableUnison(false)
		, Spread(0.5f)
		, Pan(0.0f)
		, LFO1Frequency(1.0f)
		, LFO1Gain(0.0f)
		, LFO1Type(ESynthLFOType::Sine)
		, LFO1Mode(ESynthLFOMode::Sync)
		, LFO1PatchType(ESynthLFOPatchType::PatchToNone)
		, LFO2Frequency(1.0f)
		, LFO2Gain(0.0f)
		, LFO2Type(ESynthLFOType::Sine)
		, LFO2Mode(ESynthLFOMode::Sync)
		, LFO2PatchType(ESynthLFOPatchType::PatchToNone)
		, GainDb(-3.0f)
		, AttackTime(10.0f)
		, DecayTime(100.0f)
		, SustainGain(0.707f)
		, ReleaseTime(5000.0f)
		, EnvModPatchType(ESynthEnvModPatch::PatchToNone)
		, bInvertModEnv(false)
		, bLegato(true)
		, bRetrigger(false)
		, FilterFrequency(8000.0f)
		, FilterQ(2.0f)
		, FilterType(ESynthFilterType::LowPass)
		, FilterAlgorithm(ESynthFilterAlgorithm::Ladder)
		, bStereoDelayEnabled(true)
		, StereoDelayMode(ESynthStereoDelayMode::PingPong)
		, StereoDelayTime(700.0f)
		, StereoDelayFeedback(0.7f)
		, StereoDelayWetlevel(0.3f)
		, StereoDelayRatio(0.2f)
		, bChorusEnabled(false)
		, ChorusDepth(0.2f)
		, ChorusFeedback(0.5f)
		, ChorusFrequency(2.0f)
	{}
};

USTRUCT(BlueprintType)
struct SYNTHESIS_API FEpicSynth1PresetBankEntry
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	FString PresetName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
	FEpicSynth1Preset Preset;

	FEpicSynth1PresetBankEntry()
		: PresetName(TEXT("Init"))
	{}
};


UCLASS(ClassGroup = Synth, meta = (BlueprintSpawnableComponent))
class SYNTHESIS_API UEpicSynth1PresetBank : public UObject
{
	GENERATED_BODY()

	UEpicSynth1PresetBank(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}

public:

 	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Synth|Preset")
 	TArray<FEpicSynth1PresetBankEntry> Presets;
};


/**
* USynth1Component
* Implementation of a USynthComponent.
*/
UCLASS(ClassGroup = Synth, meta = (BlueprintSpawnableComponent))
class SYNTHESIS_API UEpicSynth1Component : public USynthComponent
{
	GENERATED_BODY()

	UEpicSynth1Component(const FObjectInitializer& ObjectInitializer);

	// Returns the number of channels (1 or 2) you would like your synth to run with. Called only once at init of synth component.
	virtual int32 GetNumChannels() const override { return 2; }

	// Called to generate more audio
	virtual void OnGenerateAudio(TArray<float>& OutAudio) override;

	//// Define synth parameter functions

	// Play a new note. Optionally pass in a duration to automatically turn off the note.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void NoteOn(const float Note, const int32 Velocity, const float Duration = -1.0f);

	// Stop the note (will only do anything if a voice is playing with that note)
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void NoteOff(const float Note, const bool bAllNotesOff);

	// Sets whether or not to use polyphony for the synthesizer.
	// @param bEnablePolyphony Whether or not to enable polyphony for the synth.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetEnablePolyphony(bool bEnablePolyphony);

	// Set the oscillator type. 
	// @param OscIndex Which oscillator to set the type for.
	// @param OscType The oscillator type to set.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetOscType(int32 OscIndex, ESynth1OscType OscType);

	// Sets the oscillator octaves
	// @param OscIndex Which oscillator to set the type for.
	// @param Octave Which octave to set the oscillator to (relative to base frequency/pitch).
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetOscOctave(int32 OscIndex, float Octave);

	// Sets the oscillator semitones.
	// @param OscIndex Which oscillator to set the type for.
	// @param Semitones The amount of semitones to set the oscillator to (relative to base frequency/pitch).
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetOscSemitones(int32 OscIndex, float Semitones);

	// Sets the oscillator cents.
	// @param OscIndex Which oscillator to set the type for.
	// @param Cents The amount of cents to set the oscillator to (relative to base frequency/pitch)..
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetOscCents(int32 OscIndex, float Cents);

	// Sets the synth pitch bend amount.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetPitchBend(float PitchBend);

	// Sets the synth portamento [0.0, 1.0]
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetPortamento(float Portamento);

	// Sets the square wave pulsewidth [0.0, 1.0]
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetOscPulsewidth(int32 OscIndex, float Pulsewidth);

	// Sets whether or not the synth is in unison mode (i.e. no spread)
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetEnableUnison(bool EnableUnison);

	// Sets the pan of the synth [-1.0, 1.0].
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetPan(float Pan);

	// Sets the amount of spread of the oscillators. [0.0, 1.0]
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetSpread(float Spread);

	// Sets the LFO frequency in hz
	// @param LFOIndex Which LFO to set the frequency for.
	// @param FrequencyHz The LFO frequency to use.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetLFOFrequency(int32 LFOIndex, float FrequencyHz);

	// Sets the LFO gain factor
	// @param LFOIndex Which LFO to set the frequency for.
	// @param Gain The gain factor to use for the LFO.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetLFOGain(int32 LFOIndex, float Gain);

	// Sets the LFO type
	// @param LFOIndex Which LFO to set the frequency for.
	// @param LFOType The LFO type to use.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetLFOType(int32 LFOIndex, ESynthLFOType LFOType);

	// Sets the LFO type
	// @param LFOIndex Which LFO to set the frequency for.
	// @param LFOMode The LFO mode to use.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetLFOMode(int32 LFOIndex, ESynthLFOMode LFOMode);

	// Sets the LFO patch type. LFO patch determines what parameter is modulated by the LFO.
	// @param LFOIndex Which LFO to set the frequency for.
	// @param LFOPatchType The LFO patch type to use.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetLFOPatch(int32 LFOIndex, ESynthLFOPatchType LFOPatchType);

	// Sets the synth gain in decibels.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetGainDb(float GainDb);

	// Sets the envelope attack time in msec.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetAttackTime(float AttackTimeMsec);

	// Sets the envelope decay time in msec.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetDecayTime(float DecayTimeMsec);

	// Sets the envelope sustain gain value.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetSustainGain(float SustainGain);

	// Sets the envelope release time in msec.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetReleaseTime(float ReleaseTimeMsec);

	// Sets whether or not to modulate a param based on the envelope. Uses bias envelope output (offset from sustain gain).
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetModEnvPatch(const ESynthEnvModPatch InPatchType);
	
	// Sets whether or not to invert the envelope modulator.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetModEnvInvert(const bool bInInvert);

	// Sets whether or not to use legato for the synthesizer.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetEnableLegato(bool LegatoEnabled);

	// Sets whether or not to retrigger envelope per note on.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetEnableRetrigger(bool RetriggerEnabled);

	// Sets the filter cutoff frequency in hz.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetFilterFrequency(float FilterFrequencyHz);

	// Sets the filter Q (resonance)
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetFilterQ(float FilterQ);

	// Sets the filter type.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetFilterType(ESynthFilterType FilterType);

	// Sets the filter algorithm.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetFilterAlgorithm(ESynthFilterAlgorithm FilterAlgorithm);

	// Sets whether not stereo delay is enabled.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetStereoDelayIsEnabled(bool StereoDelayEnabled);

	// Sets whether not stereo delay is enabled.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetStereoDelayMode(ESynthStereoDelayMode StereoDelayMode);

	// Sets the amount of stereo delay time in msec.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetStereoDelayTime(float DelayTimeMsec);

	// Sets the amount of stereo delay feedback [0.0, 1.0]
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetStereoDelayFeedback(float DelayFeedback);

	// Sets the amount of stereo delay wetlevel [0.0, 1.0]
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetStereoDelayWetlevel(float DelayWetlevel);

	// Sets the amount of stereo delay ratio between left and right delay lines [0.0, 1.0]
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetStereoDelayRatio(float DelayRatio);

	// Sets whether or not chorus is enabled.
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetChorusEnabled(bool EnableChorus);

	// Sets the chorus depth
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetChorusDepth(float Depth);

	// Sets the chorus feedback
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetChorusFeedback(float Feedback);

	// Sets the chorus frequency
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetChorusFrequency(float Frequency);

	// Sets the preset struct for the synth
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	void SetSynthPreset(const FEpicSynth1Preset& SynthPreset);

	// Creates a new modular synth patch between a modulation source and a set of modulation destinations
	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	FPatchId CreatePatch(const ESynth1PatchSource PatchSource, const TArray<FSynth1PatchCable>& PatchCables, const bool bEnableByDefault);

	UFUNCTION(BlueprintCallable, Category = "Synth|Components|Audio")
	bool SetEnablePatch(const FPatchId PatchId, const bool bIsEnabled);

protected:

	Audio::FEpicSynth1 EpicSynth1;
};
