// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "DSP/SampleBuffer.h"
#include "AudioMixer.h"

namespace Audio
{
	FSampleBuffer::FSampleBuffer()
		: AudioDevice(nullptr)
		, SoundWave(nullptr)
		, bIsLoading(false)
	{
	}

	FSampleBuffer::~FSampleBuffer()
	{
	}

	void FSampleBuffer::Init(FAudioDevice* InAudioDevice)
	{
		AudioDevice = InAudioDevice;
	}

	void FSampleBuffer::LoadSoundWave(USoundWave* InSoundWave, const bool bLoadAsync)
	{
		check(InSoundWave);

		if (AudioDevice)
		{
			// finish loading any other sound waves before flushing this
			if (bIsLoading)
			{
				check(SoundWave);
				if (SoundWave->AudioDecompressor)
				{	
					SoundWave->AudioDecompressor->EnsureCompletion();
					delete SoundWave->AudioDecompressor;
					SoundWave->AudioDecompressor = nullptr;
				}
			}

			SoundWave = InSoundWave;

			bIsLoaded = false;
			bIsLoading = true;

			// Precache the sound wave and force it to be fully decompressed
			AudioDevice->Precache(SoundWave, !bLoadAsync, true, true);

			// If we synchronously loaded then we're good
			if (!bLoadAsync)
			{
				check(SoundWave->RawPCMData);
				bIsLoading = false;
				bIsLoaded = true;
			}
		}
	}

	void FSampleBuffer::UpdateLoading()
	{
		if (bIsLoading)
		{
			check(SoundWave);

			if (SoundWave->AudioDecompressor && SoundWave->AudioDecompressor->IsDone())
			{
				bIsLoading = false;
				bIsLoaded = true;

				delete SoundWave->AudioDecompressor;
				SoundWave->AudioDecompressor = nullptr;
			}
		}
	}

	bool FSampleBuffer::IsLoaded() const
	{
		return bIsLoaded;
	}

	bool FSampleBuffer::IsLoading() const
	{
		return bIsLoading;
	}

	const int16* FSampleBuffer::GetData() const
	{
		if (bIsLoaded)
		{
			return (int16*)SoundWave->RawPCMData;
		}
		return nullptr;
	}

	int32 FSampleBuffer::GetNumSamples() const
	{
		if (bIsLoaded)
		{
			return SoundWave->RawPCMDataSize / sizeof(int16);
		}
		return 0;
	}
	
	int32 FSampleBuffer::GetNumFrames() const
	{
		if (bIsLoaded)
		{
			return GetNumSamples() / GetNumChannels();
		}
		return 0;
	}

	int32 FSampleBuffer::GetNumChannels() const
	{
		if (bIsLoaded)
		{
			return SoundWave->NumChannels;
		}
		return 0;
	}

	int32 FSampleBuffer::GetSampleRate() const
	{
		if (bIsLoaded)
		{
			return SoundWave->SampleRate;
		}
		return 0;
	}
}


