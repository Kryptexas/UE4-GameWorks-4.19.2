// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DSP/Dsp.h"
#include "Sound/SoundWave.h"
#include "AudioDevice.h"

namespace Audio
{
	// An object which fully loads/decodes a sound wave and allows direct access to sound wave data.	
	class AUDIOMIXER_API FSampleBuffer
	{
	public:
		FSampleBuffer();
		~FSampleBuffer();

		// Initialize the sample buffer
		void Init(FAudioDevice* InAudioDevice);

		// Load a USoundWave and get the decoded PCM data and store internally
		void LoadSoundWave(USoundWave* InSoundWave, const bool bLoadAsync = true);

		// Updates the loading state
		void UpdateLoading();

		// Queries if the sound file is loaded
		bool IsLoaded() const;

		// Queries if the sound file is loading
		bool IsLoading() const;

		// Gets the raw PCM data of the sound wave
		const int16* GetData() const;

		// Gets the number of samples of the sound wave
		int32 GetNumSamples() const;

		// Gets the number of frames of the sound wave
		int32 GetNumFrames() const;

		// Gets the number of channels of the sound wave
		int32 GetNumChannels() const;

		// Gets the sample rate of the sound wave
		int32 GetSampleRate() const;

	protected:
		
		// What audio device this sample buffer is playing in.
		FAudioDevice* AudioDevice;

		// Sound wave object
		USoundWave* SoundWave;

		bool bIsLoaded;
		bool bIsLoading;
	};

}

