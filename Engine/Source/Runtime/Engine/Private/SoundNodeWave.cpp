// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"
#include "AudioDecompress.h"
#include "SoundDefinitions.h"
#include "SubtitleManager.h"
#include "AudioDerivedData.h"
#include "TargetPlatform.h"

/*-----------------------------------------------------------------------------
	USoundNodeWave implementation.
-----------------------------------------------------------------------------*/
UDEPRECATED_SoundNodeWave::UDEPRECATED_SoundNodeWave(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Volume = 0.75;
	Pitch = 1.0;
	CompressionQuality = 40;
	bLoopingSound = true;
}

void UDEPRECATED_SoundNodeWave::Serialize( FArchive& Ar )
{
	Super::Serialize( Ar );

	bool bCooked = false;
	if (Ar.UE4Ver() >= VER_UE4_ADD_COOKED_TO_SOUND_NODE_WAVE)
	{
		bCooked = Ar.IsCooking();
		Ar << bCooked;
	}

	if (FPlatformProperties::RequiresCookedData() && !bCooked && Ar.IsLoading())
	{
		UE_LOG(LogAudio, Fatal, TEXT("This platform requires cooked packages, and audio data was not cooked into %s."), *GetFullName());
	}

	if (bCooked)
	{
		if (!Ar.IsCooking())
		{
			// CompressedFormatData is unneeded to migrate data to SoundWave
			FFormatContainer CompressedFormatData;
			CompressedFormatData.Serialize(Ar, this);
		}
	}
	else
	{
		// only save the raw data for non-cooked packages
		RawData.Serialize( Ar, this );
	}

	if( Ar.UE4Ver() < VER_UE4_ADD_SOUNDNODEWAVE_TO_DDC )
	{
		FByteBulkData UnusedCompressedPCData;
		UnusedCompressedPCData.Serialize( Ar, this );

		FByteBulkData UnusedCompressedXbox360Data;
		UnusedCompressedXbox360Data.Serialize( Ar, this );

		FByteBulkData UnusedCompressedPS3Data;
		UnusedCompressedPS3Data.Serialize( Ar, this );

		FByteBulkData UnusedCompressedWiiUData;
		UnusedCompressedWiiUData.Serialize( Ar, this );

		FByteBulkData UnusedCompressedIPhoneData;
		UnusedCompressedIPhoneData.Serialize( Ar, this );
	}

	if( Ar.UE4Ver() >= VER_UE4_ADD_SOUNDNODEWAVE_GUID )
	{
		// CompressedDataGuid is unneeded to migrate data to SoundWave
		FGuid CompressedDataGuid;
		Ar << CompressedDataGuid;
	}
}
