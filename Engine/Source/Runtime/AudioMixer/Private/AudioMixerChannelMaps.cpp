// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AudioMixer.h"
#include "AudioMixerDevice.h"
#include "Misc/ConfigCacheIni.h"

namespace Audio
{
	// Tables based on Ac-3 down-mixing
	// Rows: output speaker configuration
	// Cols: input source channels

	static const int32 MaxOutputChannels = 8;

	static float ToMonoMatrix[MaxOutputChannels * 1] =
	{
		// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight  
		0.707f,			0.707f,		1.0f,		0.0f,			0.5f,		0.5f,		0.5f,		0.5f,		// FrontLeft
	};

	static float ToStereoMatrix[MaxOutputChannels * 2] =
	{
		// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight  
		1.0f,			0.0f,		0.707f,		0.0f,			0.707f,		0.0f,		0.707f,		0.0f,		// FrontLeft
		0.0f,			1.0f,		0.707f,		0.0f,			0.0f,		0.707f,		0.0f,		0.707f,		// FrontRight
	};

	static float ToTriMatrix[MaxOutputChannels * 3] =
	{
		// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight  
		1.0f,			0.0f,		0.0f,		0.0f,			0.707f,		0.0f,		0.707f,		0.0f,		// FrontLeft
		0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.707f,		0.0f,		0.707f,		// FrontRight
		0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// Center
	};

	static float ToQuadMatrix[MaxOutputChannels * 4] =
	{
		// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight	
		1.0f,			0.0f,		0.707f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// FrontLeft
		0.0f,			1.0f,		0.707f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// FrontRight
		0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		1.0f,		0.0f,		// SideLeft
		0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		0.0f,		1.0f,		// SideRight
	};

	static float To5Matrix[MaxOutputChannels * 5] =
	{
		// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight	
		1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// FrontLeft
		0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// FrontRight
		0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// Center
		0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		1.0f,		0.0f,		// SideLeft
		0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		0.0f,		1.0f,		// SideRight
	};

	static float To5Point1Matrix[MaxOutputChannels * 6] =
	{
		// FrontLeft	FrontRight	Center		LowFrequency	SideLeft	SideRight	BackLeft	BackRight	
		1.0f,			0.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// FrontLeft
		0.0f,			1.0f,		0.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// FrontRight
		0.0f,			0.0f,		1.0f,		0.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// Center
		0.0f,			0.0f,		0.0f,		1.0f,			0.0f,		0.0f,		0.0f,		0.0f,		// LowFrequency
		0.0f,			0.0f,		0.0f,		0.0f,			1.0f,		0.0f,		1.0f,		0.0f,		// SideLeft
		0.0f,			0.0f,		0.0f,		0.0f,			0.0f,		1.0f,		0.0f,		1.0f,		// SideRight
	};

	static float ToHexMatrix[MaxOutputChannels * 7] =
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

	// NOTE: the BackLeft/BackRight and SideLeft/SideRight are reversed than they should be since our 7.1 importer code has it backward
	static float To7Point1Matrix[MaxOutputChannels * 8] =
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

	static float* OutputChannelMaps[MaxOutputChannels] =
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

	TMap<int32, TArray<float>> FMixerDevice::ChannelMapCache;

	int32 FMixerDevice::GetChannelMapCacheId(const int32 NumSourceChannels, const int32 NumOutputChannels) const
	{
		// Just create a unique number for source and output channel combination
		return NumSourceChannels + 10 * NumOutputChannels;
	}

	void FMixerDevice::Get2DChannelMap(const int32 NumSourceChannels, const int32 NumOutputChannels, TArray<float>& OutChannelMap) const
	{
		if (NumSourceChannels > 8 || NumOutputChannels > 8)
		{
			// Return a zero'd channel map buffer in the case of an unsupported channel configuration
			OutChannelMap.AddZeroed(NumSourceChannels * NumOutputChannels);
			UE_LOG(LogAudioMixer, Warning, TEXT("Unsupported source channel (%d) count or output channels (%d)"), NumSourceChannels, NumOutputChannels);
			return;
		}

		const int32 CacheID = GetChannelMapCacheId(NumSourceChannels, NumOutputChannels);
		const TArray<float>* CachedChannelMap = ChannelMapCache.Find(CacheID);
		if (!CachedChannelMap)
		{
			Get2DChannelMapInternal(NumSourceChannels, NumOutputChannels, OutChannelMap);
		}
		else
		{
			OutChannelMap = *CachedChannelMap;
		}
	}

	const float* FMixerDevice::Get2DChannelMap(const int32 NumSourceChannels, const int32 NumOutputChannels) const
	{
		if (NumSourceChannels > 8 || NumOutputChannels > 8)
		{
			UE_LOG(LogAudioMixer, Warning, TEXT("Unsupported source channel (%d) count or output channels (%d)"), NumSourceChannels, NumOutputChannels);
			return nullptr;
		}

		const int32 CacheID = GetChannelMapCacheId(NumSourceChannels, NumOutputChannels);
		const TArray<float>* CachedChannelMap = ChannelMapCache.Find(CacheID);
		if (CachedChannelMap)
		{
			return CachedChannelMap->GetData();
		}

		return nullptr;
	}


	void FMixerDevice::Get2DChannelMapInternal(const int32 NumSourceChannels, const int32 NumOutputChannels, TArray<float>& OutChannelMap) const
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
				// Mapping out to more than 2 channels, mono sources should be equally spread to left and right
				OutChannelMap.Add(0.707f);
				OutChannelMap.Add(0.707f);

				for (int32 OutputChannel = 2; OutputChannel < NumOutputChannels; ++OutputChannel)
				{
					const int32 Index = OutputChannel * MaxOutputChannels;
					OutChannelMap.Add(Matrix[Index]);
				}
			}
		}
		else
		{
			for (int32 SourceChannel = 0; SourceChannel < NumSourceChannels; ++SourceChannel)
			{
				for (int32 OutputChannel = 0; OutputChannel < NumOutputChannels; ++OutputChannel)
				{
					const int32 Index = OutputChannel * MaxOutputChannels + SourceChannel;
					OutChannelMap.Add(Matrix[Index]);
				}
			}
		}
	}

	void FMixerDevice::CacheChannelMap(const int32 NumSourceChannels, const int32 NumOutputChannels)
	{
		// Generate the unique cache ID for the channel count configuration
		const int32 CacheID = GetChannelMapCacheId(NumSourceChannels, NumOutputChannels);
		TArray<float> ChannelMap;
		Get2DChannelMapInternal(NumSourceChannels, NumOutputChannels, ChannelMap);
		ChannelMapCache.Add(CacheID, ChannelMap);
	}

	void FMixerDevice::InitializeChannelMaps()
	{	
		// If we haven't yet created the static channel map cache
		if (!ChannelMapCache.Num())
		{
			// Loop through all input to output channel map configurations and cache them
			for (int32 InputChannelCount = 1; InputChannelCount < 9; ++InputChannelCount)
			{
				for (int32 OutputChannelCount = 1; OutputChannelCount < 9; ++OutputChannelCount)
				{
					CacheChannelMap(InputChannelCount, OutputChannelCount);
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

		// Ignore front cent and low-frequency for azimuth computations. Both of these channels are manual opt-ins from data and not directly used in 3d audio calculations
		DefaultChannelAzimuthPosition[EAudioMixerChannel::FrontCenter] = { EAudioMixerChannel::FrontCenter, -1 };
		DefaultChannelAzimuthPosition[EAudioMixerChannel::LowFrequency] = { EAudioMixerChannel::LowFrequency, -1 };

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

		// Build a map of azimuth positions of only the current audio device's output channels
		CurrentChannelAzimuthPositions.Reset();
		for (EAudioMixerChannel::Type Channel : PlatformInfo.OutputChannelArray)
		{
			// Only track non-LFE and non-Center channel azimuths for use with 3d channel mappings
			if (Channel != EAudioMixerChannel::LowFrequency && Channel != EAudioMixerChannel::FrontCenter)
			{
				++NumSpatialChannels;
				CurrentChannelAzimuthPositions.Add(DefaultChannelAzimuthPosition[Channel]);
			}
		}

		check(NumSpatialChannels > 0);
		OmniPanFactor = 1.0f / FMath::Sqrt(NumSpatialChannels);

		// Sort the current mapping by azimuth
		struct FCompareByAzimuth
		{
			FORCEINLINE bool operator()(const FChannelPositionInfo& A, const FChannelPositionInfo& B) const
			{
				return A.Azimuth < B.Azimuth;
			}
		};

		CurrentChannelAzimuthPositions.Sort(FCompareByAzimuth());
	}
}