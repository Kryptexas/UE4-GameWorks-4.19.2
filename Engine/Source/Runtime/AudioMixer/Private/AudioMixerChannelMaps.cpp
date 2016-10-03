// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AudioMixerPCH.h"
#include "AudioMixerDevice.h"

namespace Audio
{
	/** Default 2D channel maps from input channels to output channels */

	static float MonoToStereoMatrix[1 * 2] =
	{
		// FrontLeft
		0.707f,		// FrontLeft
		0.707f,		// FrontRight
	};

	static float MonoToQuadMatrix[1 * 4] =
	{
		// FrontLeft
		0.707f,		// FrontLeft
		0.707f,		// FrontRight
		0.0f,		// BackLeft
		0.0f,		// BackRight
	};

	static float MonoTo51Matrix[1 * 6] =
	{
		// FrontLeft
		0.707f,		// FrontLeft
		0.707f,		// FrontRight
		0.0f,		// FrontCenter
		0.0f,		// LowFrequency
		0.0f,		// BackLeft
		0.0f,		// BackRight
	};

	static float MonoTo71Matrix[1 * 8] =
	{
		// FrontLeft
		0.707f,		// FrontLeft
		0.707f,		// FrontRight
		0.0f,		// FrontCenter
		0.0f,		// LowFrequency
		0.0f,		// BackLeft
		0.0f,		// BackRight
		0.0f,		// SideLeft
		0.0f,		// SideRight
	};

	static float StereoToStereoMatrix[2 * 2] =
	{
		// FrontLeft		FrontRight
		1.0f,				0.0f,		// FrontLeft
		0.0f,				1.0f,		// FrontRight
	};

	static float StereoToQuadMatrix[2 * 4] =
	{
		// FrontLeft		FrontRight
		0.707f,				0.0f,		// FrontLeft
		0.0f,				0.707f,		// FrontRight
		0.0f,				0.707f,		// BackLeft
		0.707f,				0.0f,		// BackRight
	};

	static float StereoTo51Matrix[2 * 6] =
	{
		// FrontLeft		FrontRight
		0.707f,				0.0f,		// FrontLeft
		0.0f,				0.707f,		// FrontRight
		0.0f,				0.0f,		// FrontCenter
		0.0f,				0.0f,		// LowFrequency
		0.0f,				0.707f,		// BackLeft
		0.707f,				0.0f,		// BackRight
	};

	static float StereoTo71Matrix[2 * 8] =
	{
		// FrontLeft		FrontRight
		0.707f,				0.0f,		// FrontLeft
		0.0f,				0.707f,		// FrontRight
		0.0f,				0.0f,		// FrontCenter
		0.0f,				0.0f,		// LowFrequency
		0.0f,				0.707f,		// BackLeft
		0.707f,				0.0f,		// BackRight
		0.0f,				0.0f,		// SideLeft
		0.0f,				0.0f,		// SideRight
	};

	static float QuadToStereoMatrix[4 * 2] =
	{
		// FrontLeft		FrontRight		BackLeft		BackRight
		0.5f,				0.0f,			0.5f,			0.0f,			// FrontLeft
		0.0f,				0.5f,			0.0f,			0.5f,			// FrontRight
	};

	static float QuadToQuadMatrix[4 * 4] =
	{
		// FrontLeft		FrontRight		BackLeft		BackRight
		1.0f,				0.0f,			0.0f,			0.0f,			// FrontLeft
		0.0f,				1.0f,			0.0f,			0.0f,			// FrontRight
		0.0f,				0.0f,			1.0f,			0.0f,			// BackLeft
		0.0f,				0.0f,			0.0f,			1.0f,			// BackRight
	};

	static float QuadTo51Matrix[4 * 6] =
	{
		// FrontLeft		FrontRight		BackLeft		BackRight
		1.0f,				0.0f,			0.0f,			0.0f,			// FrontLeft
		0.0f,				1.0f,			0.0f,			0.0f,			// FrontRight
		0.0f,				0.0f,			0.0f,			0.0f,			// FrontCenter
		0.0f,				0.0f,			0.0f,			0.0f,			// LowFrequency
		0.0f,				0.0f,			1.0f,			0.0f,			// BackLeft
		0.0f,				0.0f,			0.0f,			1.0f,			// BackRight
	};

	static float QuadTo71Matrix[4 * 8] =
	{
		// FrontLeft		FrontRight		BackLeft		BackRight
		1.0f,				0.0f,			0.0f,			0.0f,			// FrontLeft
		0.0f,				1.0f,			0.0f,			0.0f,			// FrontRight
		0.0f,				0.0f,			0.0f,			0.0f,			// FrontCenter
		0.0f,				0.0f,			0.0f,			0.0f,			// LowFrequency
		0.0f,				0.0f,			1.0f,			0.0f,			// BackLeft
		0.0f,				0.0f,			0.0f,			1.0f,			// BackRight
		0.0f,				0.0f,			0.0f,			0.0f,			// SideLeft
		0.0f,				0.0f,			0.0f,			0.0f,			// SideRight
	};

	static float HexToStereoMatrix[6 * 2] =
	{
		// FrontLeft		FrontRight		FrontCenter		LowFrequency		BackLeft		BackRight
		0.577f,				0.0f,			0.577f,			0.0f,				0.577f,			0.0f,			// FrontLeft
		0.0f,				0.577f,			0.577f,			0.0f,				0.577f,			0.577f,			// FrontRight
	};

	static float HexToQuadMatrix[6 * 4] =
	{
		// FrontLeft		FrontRight		FrontCenter		LowFrequency		BackLeft		BackRight
		0.577f,				0.0f,			0.577f,			0.0f,				0.0f,			0.0f,			// FrontLeft
		0.0f,				0.577f,			0.577f,			0.0f,				0.0f,			0.0f,			// FrontRight
		0.0f,				0.0f,			0.0f,			0.0f,				0.577f,			0.0f,			// BackLeft
		0.0f,				0.0f,			0.0f,			0.0f,				0.0f,			0.577f,			// BackRight
	};

	static float HexTo51Matrix[6 * 6] =
	{
		// FrontLeft		FrontRight		FrontCenter		LowFrequency		BackLeft		BackRight
		1.0f,				0.0f,			0.0f,			0.0f,				0.0f,			0.0f,			// FrontLeft
		0.0f,				1.0f,			0.0f,			0.0f,				0.0f,			0.0f,			// FrontRight
		0.0f,				0.0f,			1.0f,			0.0f,				0.0f,			0.0f,			// FrontCenter
		0.0f,				0.0f,			0.0f,			1.0f,				0.0f,			0.0f,			// LowFrequency
		0.0f,				0.0f,			0.0f,			0.0f,				1.0f,			0.0f,			// BackLeft
		0.0f,				0.0f,			0.0f,			0.0f,				0.0f,			1.0f,			// BackRight
	};

	static float HexTo71Matrix[6 * 8] =
	{
		// FrontLeft		FrontRight		FrontCenter		LowFrequency		BackLeft		BackRight
		1.0f,				0.0f,			0.0f,			0.0f,				0.0f,			0.0f,			// FrontLeft
		0.0f,				1.0f,			0.0f,			0.0f,				0.0f,			0.0f,			// FrontRight
		0.0f,				0.0f,			1.0f,			0.0f,				0.0f,			0.0f,			// FrontCenter
		0.0f,				0.0f,			0.0f,			1.0f,				0.0f,			0.0f,			// LowFrequency
		0.0f,				0.0f,			0.0f,			0.0f,				1.0f,			0.0f,			// BackLeft
		0.0f,				0.0f,			0.0f,			0.0f,				0.0f,			1.0f,			// BackRight
		0.0f,				0.0f,			0.0f,			0.0f,				0.0f,			0.0f,			// SideLeft
		0.0f,				0.0f,			0.0f,			0.0f,				0.0f,			0.0f,			// SideRight
	};

	static inline int32 GetChannelMapID(const int32 InputChannels, const int32 OutputChannels)
	{
		return InputChannels + 100 * OutputChannels;
	}

	void FMixerDevice::Get2DChannelMap(const int32 NumSourceChannels, TArray<float>& OutChannelMap)
	{
		int32 ChannelMapIndex = GetChannelMapID(NumSourceChannels, PlatformInfo.NumChannels);
		FChannelMapEntry* Entry = ChannelMaps.Find(ChannelMapIndex);
		if (!Entry)
		{
			OutChannelMap.AddZeroed(PlatformInfo.NumChannels);
			UE_LOG(LogAudioMixer, Warning, TEXT("Unsupported source channel count or output channels %d"), NumSourceChannels);
			return;
		}

		OutChannelMap = Entry->ChannelWeights;
	}

	void FMixerDevice::Initialize2DChannelMaps()
	{
		ChannelMaps.Reset();

		ChannelMaps.Add(GetChannelMapID(1, 2), FChannelMapEntry(EChannelMap::MonoToStereo,	MonoToStereoMatrix, ARRAY_COUNT(MonoToStereoMatrix)));
		ChannelMaps.Add(GetChannelMapID(1, 4), FChannelMapEntry(EChannelMap::MonoToQuad,	MonoToQuadMatrix,	ARRAY_COUNT(MonoToQuadMatrix)));
		ChannelMaps.Add(GetChannelMapID(1, 6), FChannelMapEntry(EChannelMap::MonoTo51,		MonoTo51Matrix,		ARRAY_COUNT(MonoTo51Matrix)));
		ChannelMaps.Add(GetChannelMapID(1, 8), FChannelMapEntry(EChannelMap::MonoTo71,		MonoTo71Matrix,		ARRAY_COUNT(MonoTo71Matrix)));

		ChannelMaps.Add(GetChannelMapID(2, 2), FChannelMapEntry(EChannelMap::StereoToStereo, StereoToStereoMatrix,	ARRAY_COUNT(StereoToStereoMatrix)));
		ChannelMaps.Add(GetChannelMapID(2, 4), FChannelMapEntry(EChannelMap::StereoToQuad,	StereoToQuadMatrix,		ARRAY_COUNT(StereoToQuadMatrix)));
		ChannelMaps.Add(GetChannelMapID(2, 6), FChannelMapEntry(EChannelMap::StereoTo51,	StereoTo51Matrix,		ARRAY_COUNT(StereoTo51Matrix)));
		ChannelMaps.Add(GetChannelMapID(2, 8), FChannelMapEntry(EChannelMap::StereoTo71,	StereoTo71Matrix,		ARRAY_COUNT(StereoTo71Matrix)));

		ChannelMaps.Add(GetChannelMapID(4, 2), FChannelMapEntry(EChannelMap::QuadToStereo,	QuadToStereoMatrix,	ARRAY_COUNT(QuadToStereoMatrix)));
		ChannelMaps.Add(GetChannelMapID(4, 4), FChannelMapEntry(EChannelMap::QuadToQuad,	QuadToQuadMatrix,	ARRAY_COUNT(QuadToQuadMatrix)));
		ChannelMaps.Add(GetChannelMapID(4, 6), FChannelMapEntry(EChannelMap::QuadTo51,		QuadTo51Matrix,		ARRAY_COUNT(QuadTo51Matrix)));
		ChannelMaps.Add(GetChannelMapID(4, 8), FChannelMapEntry(EChannelMap::QuadTo71,		QuadTo71Matrix,		ARRAY_COUNT(QuadTo71Matrix)));

		ChannelMaps.Add(GetChannelMapID(6, 2), FChannelMapEntry(EChannelMap::HexToStereo,	HexToStereoMatrix,	ARRAY_COUNT(HexToStereoMatrix)));
		ChannelMaps.Add(GetChannelMapID(6, 4), FChannelMapEntry(EChannelMap::HexToQuad,		HexToQuadMatrix,	ARRAY_COUNT(HexToQuadMatrix)));
		ChannelMaps.Add(GetChannelMapID(6, 6), FChannelMapEntry(EChannelMap::HexTo51,		HexTo51Matrix,		ARRAY_COUNT(HexTo51Matrix)));
		ChannelMaps.Add(GetChannelMapID(6, 8), FChannelMapEntry(EChannelMap::HexTo71,		HexTo71Matrix,		ARRAY_COUNT(HexTo71Matrix)));
	}

	void FMixerDevice::InitializeChannelAzimuthMap(const int32 NumChannels)
	{
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