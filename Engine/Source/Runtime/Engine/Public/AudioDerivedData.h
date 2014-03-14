// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DerivedDataPluginInterface.h"
#include "DerivedDataCacheInterface.h"
#include "TargetPlatform.h"


class FDerivedAudioDataCompressor : public FDerivedDataPluginInterface
{
private:
	USoundWave*			SoundNode;
	FName				Format;
	const IAudioFormat*	Compressor;

public:

	FDerivedAudioDataCompressor(USoundWave* InSoundNode, FName InFormat);

	virtual const TCHAR* GetPluginName() const OVERRIDE
	{
		return TEXT("Audio");
	}

	virtual const TCHAR* GetVersionString() const OVERRIDE
	{
		// This is a version string that mimics the old versioning scheme. If you
		// want to bump this version, generate a new guid using VS->Tools->Create GUID and
		// return it here. Ex.
		// return TEXT("855EE5B3574C43ABACC6700C4ADC62E6");
		return TEXT("0002_0000");
	}

	virtual FString GetPluginSpecificCacheKeySuffix() const OVERRIDE
	{
		int32 FormatVersion = 0xffff; // if the compressor is NULL, this will be used as the version...and in that case we expect everything to fail anyway
		if (Compressor)
		{
			FormatVersion = (int32)Compressor->GetVersion(Format);
		}
		FString FormatString = Format.ToString().ToUpper();
		check(SoundNode->CompressedDataGuid.IsValid());
		return FString::Printf( TEXT("%s_%04X_%s"), *FormatString, FormatVersion, *SoundNode->CompressedDataGuid.ToString());
	}
	
	virtual bool IsBuildThreadsafe() const OVERRIDE
	{
		return false;
	}

	virtual bool Build(TArray<uint8>& OutData) OVERRIDE;
};
