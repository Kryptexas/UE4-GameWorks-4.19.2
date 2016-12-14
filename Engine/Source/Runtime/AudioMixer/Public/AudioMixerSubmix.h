// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AudioMixer.h"
#include "AudioMixerSourceManager.h"
#include "Sound/SoundSubmix.h"

class USoundEffectSubmix;

namespace Audio
{
	class IAudioMixerEffect;
	class FMixerSourceVoice;
	class FMixerDevice;

	class FMixerSubmix
	{
	public:
		FMixerSubmix(USoundSubmix* InSoundSubmix, FMixerDevice* InMixerDevice);
		~FMixerSubmix();

		// Returns the mixer submix Id
		uint32 GetId() const { return Id; }

		// Sets the parent submix to the given submix
		void SetParentSubmix(TSharedPtr<FMixerSubmix, ESPMode::ThreadSafe> Submix);

		// Adds the given submix to this submix's children
		void AddChildSubmix(TSharedPtr<FMixerSubmix, ESPMode::ThreadSafe> Submix);

		// Gets this submix's parent submix
		TSharedPtr<FMixerSubmix, ESPMode::ThreadSafe> GetParentSubmix();

		// Returns the number of source voices currently a part of this submix.
		int32 GetNumSourceVoices() const;

		// Returns the number of wet effects in this submix.
		int32 GetNumEffects() const;

		// Add (if not already added) or sets the amount of the source voice's send amount
		void AddOrSetSourceVoice(FMixerSourceVoice* InSourceVoice, const float DryLevel, const float WetLevel);

		/** Removes the given source voice from the submix. */
		void RemoveSourceVoice(FMixerSourceVoice* InSourceVoice);

		/** Appends the effect submix to the effect submix chain. */
		void AddSoundEffectSubmix(FSoundEffectSubmix* InSoundEffectSubmix);

		// Function which processes audio.
		void ProcessAudio(TArray<float>& OutAudio);

		// Sets the wet-level to use when outputting the submix's audio to it's parent submixes (does not apply for master submix).
		void SetOutputWetLevel(const float InWetLevel);

		// Returns the submix's wet level, used by parent submixes to determine where to route output audio of children.
		float GetOutputWetLevel() const;

		// Returns the device sample rate this submix is rendering to
		int32 GetSampleRate() const;

		// Returns the output channels this submix is rendering to
		int32 GetNumOutputChannels() const;

		// Updates the submix from the main thread.
		void Update();

		// Returns the number of effects in this submix's effect chain
		int32 GetNumChainEffects() const;

		// Returns the submix effect at the given effect chain index
		FSoundEffectSubmix* GetSubmixEffect(const int32 InIndex);

	protected:
		// Down mix the given buffer to the desired down mix channel count
		void DownmixBuffer(const int32 InputChannelCount, const TArray<float>& InBuffer, const int32 DownMixChannelCount, TArray<float>& OutDownmixedBuffer);

	protected:
		// This mixer submix's Id
		uint32 Id;

		// Parent submix. 
		TSharedPtr<FMixerSubmix, ESPMode::ThreadSafe> ParentSubmix;

		// Child submixes
		TMap<uint32, TSharedPtr<FMixerSubmix, ESPMode::ThreadSafe>> ChildSubmixes;

		// The effect chain of this submix, based on the sound submix preset chain
		TArray<FSoundEffectSubmix*> EffectSubmixChain;

		// Owning mixer device. 
		FMixerDevice* MixerDevice;

		// Describes how to treat the source voice in the mixer submix's effect chain
		struct FSubmixEffectSendInfo
		{
			float DryLevel;
			float WetLevel;
		};

		// Map of mixer source voices with a given wet-level to use when processing
		TMap<FMixerSourceVoice*, FSubmixEffectSendInfo> MixerSourceVoices;

		TArray<float> DryBuffer;
		TArray<float> WetBuffer;
		TArray<float> ScratchBuffer;
		TArray<float> DownmixedBuffer;

		// The output wet level of this submix. Used by parent submix to determine how to route the output of this submix. 
		float WetLevel;
	};



}
