// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "AudioMixer.h"
#include "AudioMixerDevice.h"
#include "Misc/ConfigCacheIni.h"

namespace Audio
{
	// Tables based on Ac-3 down-mixing
	// Rows: output speaker configuration
	// Cols: input source channels

	static float ToMonoMatrix[AUDIO_MIXER_MAX_OUTPUT_CHANNELS * 1] =
	{
		// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight  
		0.707f,			0.707f,		1.0f,		0.0f,			0.5f,		0.5f,		0.5f,		0.5f,		// FrontLeft
	};

	static float VorbisToMonoMatrix[AUDIO_MIXER_MAX_OUTPUT_CHANNELS * 8] =
	{
		// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency		
		0.707f,			1.0f,		0.707f,		0.5f,			0.5f,		0.0f,		// FrontLeft
	};

	static float ToStereoMatrix[AUDIO_MIXER_MAX_OUTPUT_CHANNELS * 2] =
	{
		// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight  
		1.0f,			0.0f,		0.707f,		0.0f,			0.707f,		0.0f,		0.707f,		0.0f,		// FrontLeft
		0.0f,			1.0f,		0.707f,		0.0f,			0.0f,		0.707f,		0.0f,		0.707f,		// FrontRight
	};

	static float VorbisToStereoMatrix[AUDIO_MIXER_MAX_OUTPUT_CHANNELS * 8] =
	{
		// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency		
		1.0f,			0.707f,		0.0f,		0.707f,			0.0f,		0.0f,		// FrontLeft
		0.0f,			0.707f,		1.0f,		0.0f,			0.707f,		0.0f,		// FrontRight
	};

	static float ToTriMatrix[AUDIO_MIXER_MAX_OUTPUT_CHANNELS * 3] =
	{
		// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight  
		1.0f,			0.0f,		0.0f,		0.0f,			0.707f,		0.0f,		0.707f,		0.0f,		// FrontLeft
		0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.707f,		0.0f,		0.707f,		// FrontRight
		0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// Center
	};

	static float VorbisToTriMatrix[AUDIO_MIXER_MAX_OUTPUT_CHANNELS * 8] =
	{
		// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency		
		1.0f,			0.0f,		0.0f,		0.707f,			0.0f,		0.0f,		// FrontLeft
		0.0f,			0.0f,		1.0f,		0.0f,			0.707f,		0.0f,		// FrontRight
		0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// Center
	};

	static float ToQuadMatrix[AUDIO_MIXER_MAX_OUTPUT_CHANNELS * 4] =
	{
		// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight	
		1.0f,			0.0f,		0.707f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// FrontLeft
		0.0f,			1.0f,		0.707f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// FrontRight
		0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		1.0f,		0.0f,		// SideLeft
		0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		0.0f,		1.0f,		// SideRight
	};

	static float VorbisToQuadMatrix[AUDIO_MIXER_MAX_OUTPUT_CHANNELS * 8] =
	{
		// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency		
		1.0f,			0.707f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontLeft
		0.0f,			0.707f,		1.0f,		0.0f,			0.0f,		0.0f,		// FrontRight
		0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		// SideLeft
		0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		// SideRight
	};

	static float To5Matrix[AUDIO_MIXER_MAX_OUTPUT_CHANNELS * 5] =
	{
		// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight	
		1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// FrontLeft
		0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// FrontRight
		0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// Center
		0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		1.0f,		0.0f,		// SideLeft
		0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		0.0f,		1.0f,		// SideRight
	};

	static float VorbisTo5Matrix[AUDIO_MIXER_MAX_OUTPUT_CHANNELS * 5] =
	{
		// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency		
		1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontLeft
		0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		// FrontRight
		0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// Center
		0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		// SideLeft
		0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		// SideRight
	};

	static float To5Point1Matrix[AUDIO_MIXER_MAX_OUTPUT_CHANNELS * 6] =
	{
		// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight	
		1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// FrontLeft
		0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// FrontRight
		0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// Center
		0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// LowFrequency
		0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		1.0f,		0.0f,		// SideLeft
		0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		0.0f,		1.0f,		// SideRight
	};

	static float VorbisTo5Point1Matrix[AUDIO_MIXER_MAX_OUTPUT_CHANNELS * 6] =
	{
		// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency	
		1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontLeft
		0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		// FrontRight
		0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// Center
		0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		// LowFrequency
		0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		// SideLeft
		0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		// SideRight
	};

	static float ToHexMatrix[AUDIO_MIXER_MAX_OUTPUT_CHANNELS * 7] =
	{
		// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight	
		1.0f,			0.0f,		0.707f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// FrontLeft
		0.0f,			1.0f,		0.707f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// FrontRight
		0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		1.0f,		0.0f,		// BackLeft
		0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// LFE
		0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		1.0f,		// BackRight
		0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		0.0f,		0.0f,		// SideLeft
		0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		0.0f,		0.0f,		// SideRight
	};

	static float VorbisToHexMatrix[AUDIO_MIXER_MAX_OUTPUT_CHANNELS * 7] =
	{
		// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency	
		1.0f,			0.707f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontLeft
		0.0f,			0.707f,		1.0f,		0.0f,			0.0f,		0.0f,		// FrontRight
		0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackLeft
		0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		// LFE
		0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// BackRight
		0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		// SideLeft
		0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		// SideRight
	};

	// NOTE: the BackLeft/BackRight and SideLeft/SideRight are reversed than they should be since our 7.1 importer code has it backward
	static float To7Point1Matrix[AUDIO_MIXER_MAX_OUTPUT_CHANNELS * 8] =
	{
		// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight
		1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// FrontLeft
		0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// FrontRight
		0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// FrontCenter
		0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// LowFrequency
		0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		1.0f,		0.0f,		// BackLeft
		0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		1.0f,		// BackRight
		0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		0.0f,		0.0f,		// SideLeft
		0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		0.0f,		0.0f,		// SideRight
	};

	static float VorbisTo7Point1Matrix[AUDIO_MIXER_MAX_OUTPUT_CHANNELS * 8] =
	{
		// FrontLeft	Center		FrontRight	SideLeft		SideRight	LowFrequency		
		1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontLeft
		0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		// FrontRight
		0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// FrontCenter
		0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		// LowFrequency
		0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// SideLeft
		0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		// SideRight
		0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		// BackLeft
		0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		// BackRight
	};

	static float* OutputChannelMaps[AUDIO_MIXER_MAX_OUTPUT_CHANNELS] =
	{
		ToMonoMatrix,
		ToStereoMatrix,
		ToTriMatrix,	// Experimental
		ToQuadMatrix,
		To5Matrix,		// Experimental
		To5Point1Matrix,
		ToHexMatrix,	// Experimental
		To7Point1Matrix
	};

	// 5.1 Vorbis files have a different channel order than normal
	static float* VorbisChannelMaps[AUDIO_MIXER_MAX_OUTPUT_CHANNELS] =
	{
		VorbisToMonoMatrix,
		VorbisToStereoMatrix,
		VorbisToTriMatrix,	// Experimental
		VorbisToQuadMatrix,
		VorbisTo5Matrix,		// Experimental
		VorbisTo5Point1Matrix,
		VorbisToHexMatrix,	// Experimental
		VorbisTo7Point1Matrix
	};

	// Make a channel map cache
	static TArray<TArray<float>> ChannelMapCache;
	static TArray<TArray<float>> VorbisChannelMapCache;

	int32 FMixerDevice::GetChannelMapCacheId(const int32 NumSourceChannels, const int32 NumOutputChannels, const bool bIsCenterChannelOnly) const
	{
		int32 Index = (NumSourceChannels - 1) + AUDIO_MIXER_MAX_OUTPUT_CHANNELS * (NumOutputChannels - 1);
		if (bIsCenterChannelOnly)
		{
			Index += AUDIO_MIXER_MAX_OUTPUT_CHANNELS * AUDIO_MIXER_MAX_OUTPUT_CHANNELS;
		}
		return Index;
	}

	void FMixerDevice::Get2DChannelMap(bool bIsVorbis, const ESubmixChannelFormat InSubmixChannelType, const int32 NumSourceChannels, const bool bIsCenterChannelOnly, TArray<float>& OutChannelMap) const
	{
		int32 NumOutputChannels = GetNumChannelsForSubmixFormat(InSubmixChannelType);
		if (NumSourceChannels > 8 || NumOutputChannels > 8)
		{
			// Return a zero'd channel map buffer in the case of an unsupported channel configuration
			OutChannelMap.AddZeroed(NumSourceChannels * NumOutputChannels);
			UE_LOG(LogAudioMixer, Warning, TEXT("Unsupported source channel (%d) count or output channels (%d)"), NumSourceChannels, NumOutputChannels);
			return;
		}

		// 5.1 Vorbis files have a non-standard channel order so pick a channel map from the 5.1 vorbis channel maps based on the output channels
		if (bIsVorbis && NumSourceChannels == 6)
		{
			OutChannelMap = VorbisChannelMapCache[NumOutputChannels - 1];
		}
		else
		{
			const int32 CacheID = GetChannelMapCacheId(NumSourceChannels, NumOutputChannels, bIsCenterChannelOnly);
			OutChannelMap = ChannelMapCache[CacheID];
		}
	}

	void FMixerDevice::Get2DChannelMapInternal(const int32 NumSourceChannels, const int32 NumOutputChannels, const bool bIsCenterChannelOnly, TArray<float>& OutChannelMap) const
	{
		const int32 OutputChannelMapIndex = NumOutputChannels - 1;
		check(OutputChannelMapIndex < ARRAY_COUNT(OutputChannelMaps));

		float* Matrix = OutputChannelMaps[OutputChannelMapIndex];
		check(Matrix != nullptr);

		// Mono input sources have some special cases to take into account
		if (NumSourceChannels == 1)
		{
			// Mono-in mono-out channel map
			if (NumOutputChannels == 1)
			{
				OutChannelMap.Add(1.0f);
			}
			else
			{
				// If we have more than stereo output (means we have center channel, which is always the 3rd index)
				// Then we need to only apply 1.0 to the center channel, 0.0 for everything else
				if ((NumOutputChannels == 3 || NumOutputChannels > 4) && bIsCenterChannelOnly)
				{
					for (int32 OutputChannel = 0; OutputChannel < NumOutputChannels; ++OutputChannel)
					{
						// Center channel is always 3rd index
						if (OutputChannel == 2)
						{
							OutChannelMap.Add(1.0f);
						}
						else
						{
							OutChannelMap.Add(0.0f);
						}
					}
				}
				else
				{
					// Mapping out to more than 2 channels, mono sources should be equally spread to left and right
					OutChannelMap.Add(0.707f);
					OutChannelMap.Add(0.707f);

					for (int32 OutputChannel = 2; OutputChannel < NumOutputChannels; ++OutputChannel)
					{
						const int32 Index = OutputChannel * AUDIO_MIXER_MAX_OUTPUT_CHANNELS;
						OutChannelMap.Add(Matrix[Index]);
					}
				}
			}
		}
		else
		{
			// Compute a vorbis channel map only for 5.1 source files
			if (NumSourceChannels == 6 && !bIsCenterChannelOnly)
			{
				// Get the matrix for the channel map index
				float* VorbisMatrix = VorbisChannelMaps[OutputChannelMapIndex];

				// Get the tarray for the channel map cache
				TArray<float>& VorbisChannelMap = VorbisChannelMapCache[OutputChannelMapIndex];
				VorbisChannelMap.Reset();

				// Build it by looping over the 5.1 source channels
				for (int32 SourceChannel = 0; SourceChannel < 6; ++SourceChannel)
				{
					for (int32 OutputChannel = 0; OutputChannel < NumOutputChannels; ++OutputChannel)
					{
						const int32 Index = OutputChannel * 6 + SourceChannel;
						VorbisChannelMap.Add(VorbisMatrix[Index]);
					}
				}
			}

			for (int32 SourceChannel = 0; SourceChannel < NumSourceChannels; ++SourceChannel)
			{
				for (int32 OutputChannel = 0; OutputChannel < NumOutputChannels; ++OutputChannel)
				{
					const int32 Index = OutputChannel * AUDIO_MIXER_MAX_OUTPUT_CHANNELS + SourceChannel;
					OutChannelMap.Add(Matrix[Index]);
				}
			}
		}
	}

	void FMixerDevice::CacheChannelMap(const int32 NumSourceChannels, const int32 NumOutputChannels, const bool bIsCenterChannelOnly)
	{
		// Generate the unique cache ID for the channel count configuration
		const int32 CacheID = GetChannelMapCacheId(NumSourceChannels, NumOutputChannels, bIsCenterChannelOnly);
		Get2DChannelMapInternal(NumSourceChannels, NumOutputChannels, bIsCenterChannelOnly, ChannelMapCache[CacheID]);
	}

	void FMixerDevice::InitializeChannelMaps()
	{	
		// If we haven't yet created the static channel map cache
		if (!ChannelMapCache.Num())
		{
			// Make a matrix big enough for every possible configuration, double it to account for center channel only 
			ChannelMapCache.AddZeroed(AUDIO_MIXER_MAX_OUTPUT_CHANNELS * AUDIO_MIXER_MAX_OUTPUT_CHANNELS * 2);

			// Create a vorbis channel map cache
			VorbisChannelMapCache.AddZeroed(AUDIO_MIXER_MAX_OUTPUT_CHANNELS);

			// Loop through all input to output channel map configurations and cache them
			for (int32 InputChannelCount = 1; InputChannelCount < AUDIO_MIXER_MAX_OUTPUT_CHANNELS + 1; ++InputChannelCount)
			{
				for (int32 OutputChannelCount = 1; OutputChannelCount < AUDIO_MIXER_MAX_OUTPUT_CHANNELS + 1; ++OutputChannelCount)
				{
					CacheChannelMap(InputChannelCount, OutputChannelCount, true);
					CacheChannelMap(InputChannelCount, OutputChannelCount, false);
				}
			}
		}
	}

	void FMixerDevice::InitializeChannelAzimuthMap(const int32 NumChannels)
	{
		// Initialize and cache 2D channel maps
		InitializeChannelMaps();

		// Now setup the hard-coded values
		if (NumChannels == 2)
		{
			DefaultChannelAzimuthPosition[EAudioMixerChannel::FrontLeft] = { EAudioMixerChannel::FrontLeft, 270 };
			DefaultChannelAzimuthPosition[EAudioMixerChannel::FrontRight] = { EAudioMixerChannel::FrontRight, 90 };
		}
		else
		{
			DefaultChannelAzimuthPosition[EAudioMixerChannel::FrontLeft] = { EAudioMixerChannel::FrontLeft, 330 };
			DefaultChannelAzimuthPosition[EAudioMixerChannel::FrontRight] = { EAudioMixerChannel::FrontRight, 30 };
		}

		if (bAllowCenterChannel3DPanning)
		{
			// Allow center channel for azimuth computations
			DefaultChannelAzimuthPosition[EAudioMixerChannel::FrontCenter] = { EAudioMixerChannel::FrontCenter, 0 };
		}
		else
		{
			// Ignore front center for azimuth computations. 
			DefaultChannelAzimuthPosition[EAudioMixerChannel::FrontCenter] = { EAudioMixerChannel::FrontCenter, INDEX_NONE };
		}

		// Always ignore low frequency channel for azimuth computations. 
		DefaultChannelAzimuthPosition[EAudioMixerChannel::LowFrequency] = { EAudioMixerChannel::LowFrequency, INDEX_NONE };

		DefaultChannelAzimuthPosition[EAudioMixerChannel::BackLeft] = { EAudioMixerChannel::BackLeft, 210 };
		DefaultChannelAzimuthPosition[EAudioMixerChannel::BackRight] = { EAudioMixerChannel::BackRight, 150 };
		DefaultChannelAzimuthPosition[EAudioMixerChannel::FrontLeftOfCenter] = { EAudioMixerChannel::FrontLeftOfCenter, 15 };
		DefaultChannelAzimuthPosition[EAudioMixerChannel::FrontRightOfCenter] = { EAudioMixerChannel::FrontRightOfCenter, 345 };
		DefaultChannelAzimuthPosition[EAudioMixerChannel::BackCenter] = { EAudioMixerChannel::BackCenter, 180 };
		DefaultChannelAzimuthPosition[EAudioMixerChannel::SideLeft] = { EAudioMixerChannel::SideLeft, 270 };
		DefaultChannelAzimuthPosition[EAudioMixerChannel::SideRight] = { EAudioMixerChannel::SideRight, 90 };

		// Check any engine ini overrides for these default positions
		if (NumChannels != 2)
		{
			int32 AzimuthPositionOverride = 0;
			for (int32 ChannelOverrideIndex = 0; ChannelOverrideIndex < EAudioMixerChannel::MaxSupportedChannel; ++ChannelOverrideIndex)
			{
				EAudioMixerChannel::Type MixerChannelType = EAudioMixerChannel::Type(ChannelOverrideIndex);
				
				// Don't allow overriding the center channel if its not allowed to spatialize.
				if (MixerChannelType != EAudioMixerChannel::FrontCenter || bAllowCenterChannel3DPanning)
				{
					const TCHAR* ChannelName = EAudioMixerChannel::ToString(MixerChannelType);
					if (GConfig->GetInt(TEXT("AudioChannelAzimuthMap"), ChannelName, AzimuthPositionOverride, GEngineIni))
					{
						if (AzimuthPositionOverride >= 0 && AzimuthPositionOverride < 360)
						{
							// Make sure no channels have this azimuth angle first, otherwise we'll get some bad math later
							bool bIsUnique = true;
							for (int32 ExistingChannelIndex = 0; ExistingChannelIndex < EAudioMixerChannel::MaxSupportedChannel; ++ExistingChannelIndex)
							{
								if (DefaultChannelAzimuthPosition[ExistingChannelIndex].Azimuth == AzimuthPositionOverride)
								{
									bIsUnique = false;

									// If the override is setting the same value as our default, don't print a warning
									if (ExistingChannelIndex != ChannelOverrideIndex)
									{
										const TCHAR* ExistingChannelName = EAudioMixerChannel::ToString(EAudioMixerChannel::Type(ExistingChannelIndex));
										UE_LOG(LogAudioMixer, Warning, TEXT("Azimuth value '%d' for audio mixer channel '%s' is already used by '%s'. Azimuth values must be unique."),
											AzimuthPositionOverride, ChannelName, ExistingChannelName);
									}
									break;
								}
							}

							if (bIsUnique)
							{
								DefaultChannelAzimuthPosition[MixerChannelType].Azimuth = AzimuthPositionOverride;
							}
						}
						else
						{
							UE_LOG(LogAudioMixer, Warning, TEXT("Azimuth value, %d, for audio mixer channel %s out of range. Must be [0, 360)."), AzimuthPositionOverride, ChannelName);
						}
					}
				}
			}
		}

		// Sort the current mapping by azimuth
		struct FCompareByAzimuth
		{
			FORCEINLINE bool operator()(const FChannelPositionInfo& A, const FChannelPositionInfo& B) const
			{
				return A.Azimuth < B.Azimuth;
			}
		};

		// Build a map of azimuth positions of only the current audio device's output channels
		ChannelAzimuthPositions.Reset();

		// Setup the default channel azimuth positions
		TArray<FChannelPositionInfo> DevicePositions;
		for (EAudioMixerChannel::Type Channel : PlatformInfo.OutputChannelArray)
		{
			// Only track non-LFE and non-Center channel azimuths for use with 3d channel mappings
			if (Channel != EAudioMixerChannel::LowFrequency && DefaultChannelAzimuthPosition[Channel].Azimuth >= 0)
			{
				DevicePositions.Add(DefaultChannelAzimuthPosition[Channel]);
			}
		}
		DevicePositions.Sort(FCompareByAzimuth());
		ChannelAzimuthPositions.Add(ESubmixChannelFormat::Device, DevicePositions);
		OutputChannels[int32(ESubmixChannelFormat::Device)] = 0;

		// Now add channel mappings for the other submix types
		TArray<FChannelPositionInfo> StereoPositions;
		StereoPositions.Add(DefaultChannelAzimuthPosition[EAudioMixerChannel::FrontLeft]);
		StereoPositions.Add(DefaultChannelAzimuthPosition[EAudioMixerChannel::FrontRight]);
		StereoPositions.Sort(FCompareByAzimuth());
		ChannelAzimuthPositions.Add(ESubmixChannelFormat::Stereo, StereoPositions);
		OutputChannels[int32(ESubmixChannelFormat::Stereo)] = StereoPositions.Num();

		TArray<FChannelPositionInfo> QuadPositions;
		QuadPositions.Add(DefaultChannelAzimuthPosition[EAudioMixerChannel::FrontLeft]);
		QuadPositions.Add(DefaultChannelAzimuthPosition[EAudioMixerChannel::FrontRight]);
		QuadPositions.Add(DefaultChannelAzimuthPosition[EAudioMixerChannel::SideLeft]);
		QuadPositions.Add(DefaultChannelAzimuthPosition[EAudioMixerChannel::SideRight]);
		QuadPositions.Sort(FCompareByAzimuth());
		ChannelAzimuthPositions.Add(ESubmixChannelFormat::Quad, QuadPositions);
		OutputChannels[int32(ESubmixChannelFormat::Quad)] = QuadPositions.Num();

		TArray<FChannelPositionInfo> FiveDotOnePositions;
		FiveDotOnePositions.Add(DefaultChannelAzimuthPosition[EAudioMixerChannel::FrontLeft]);
		FiveDotOnePositions.Add(DefaultChannelAzimuthPosition[EAudioMixerChannel::FrontRight]);
		FiveDotOnePositions.Add(DefaultChannelAzimuthPosition[EAudioMixerChannel::SideLeft]);
		FiveDotOnePositions.Add(DefaultChannelAzimuthPosition[EAudioMixerChannel::SideRight]);
		FiveDotOnePositions.Sort(FCompareByAzimuth());
		ChannelAzimuthPositions.Add(ESubmixChannelFormat::FiveDotOne, FiveDotOnePositions);
		OutputChannels[int32(ESubmixChannelFormat::FiveDotOne)] = FiveDotOnePositions.Num();

		TArray<FChannelPositionInfo> SevenDotOnePositions;
		SevenDotOnePositions.Add(DefaultChannelAzimuthPosition[EAudioMixerChannel::FrontLeft]);
		SevenDotOnePositions.Add(DefaultChannelAzimuthPosition[EAudioMixerChannel::FrontRight]);
		SevenDotOnePositions.Add(DefaultChannelAzimuthPosition[EAudioMixerChannel::BackLeft]);
		SevenDotOnePositions.Add(DefaultChannelAzimuthPosition[EAudioMixerChannel::BackRight]);
		SevenDotOnePositions.Add(DefaultChannelAzimuthPosition[EAudioMixerChannel::SideLeft]);
		SevenDotOnePositions.Add(DefaultChannelAzimuthPosition[EAudioMixerChannel::SideRight]);
		SevenDotOnePositions.Sort(FCompareByAzimuth());
		ChannelAzimuthPositions.Add(ESubmixChannelFormat::SevenDotOne, SevenDotOnePositions);
		OutputChannels[int32(ESubmixChannelFormat::SevenDotOne)] = SevenDotOnePositions.Num();

		// ambisonics is special cased and uses a plugin.
		TArray<FChannelPositionInfo> FirstOrderAmbisonicsPositions;
		FirstOrderAmbisonicsPositions.Add(DefaultChannelAzimuthPosition[EAudioMixerChannel::FrontCenter]);
		FirstOrderAmbisonicsPositions.Sort(FCompareByAzimuth());
		ChannelAzimuthPositions.Add(ESubmixChannelFormat::Ambisonics, FirstOrderAmbisonicsPositions);
		OutputChannels[int32(ESubmixChannelFormat::Ambisonics)] = FirstOrderAmbisonicsPositions.Num();
	}

	int32 FMixerDevice::GetNumChannelsForSubmixFormat(const ESubmixChannelFormat InSubmixChannelType) const
	{
		if (InSubmixChannelType == ESubmixChannelFormat::Device)
		{
			return PlatformInfo.NumChannels;
		}

		return OutputChannels[(int32)InSubmixChannelType];
	}

	ESubmixChannelFormat FMixerDevice::GetSubmixChannelFormatForNumChannels(const int32 InNumChannels) const
	{
		for (int32 i = 0; i < (int32)ESubmixChannelFormat::Count; ++i)
		{
			if (OutputChannels[i] == InNumChannels)
			{
				return (ESubmixChannelFormat)i;
			}
		}
		ensureMsgf(false, TEXT("Unsupported number of submix channels %d"), InNumChannels);
		return ESubmixChannelFormat::Device;
	}

	const TArray<EAudioMixerChannel::Type>& FMixerDevice::GetChannelArrayForSubmixChannelType(const ESubmixChannelFormat InSubmixChannelType) const
	{
		if (InSubmixChannelType == ESubmixChannelFormat::Device)
		{
			return PlatformInfo.OutputChannelArray;
		}
		const TArray<EAudioMixerChannel::Type>* ChannelArray = ChannelArrays.Find(InSubmixChannelType);
		return *ChannelArray;
	}

}