// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ShaderCacheTypes.cpp: Implementation for Shader Cache specific types.
=============================================================================*/

#include "ShaderCacheTypes.h"
#include "ShaderCache.h"
#include "Serialization/MemoryWriter.h"

bool FShaderDrawKey::bTrackDrawResources = true;

FArchive& operator<<( FArchive& Ar, FShaderCaches& Info )
{
	uint32 CacheVersion = Ar.IsLoading() ? ((uint32)~0u) : ((uint32)FShaderCacheCustomVersion::Latest);
	uint32 GameVersion = Ar.IsLoading() ? ((uint32)~0u) : ((uint32)FShaderCache::GetGameVersion());
	
	Ar << CacheVersion;
	if ( !Ar.IsError() && CacheVersion == FShaderCacheCustomVersion::Latest )
	{
		Ar << GameVersion;
		
		if ( !Ar.IsError() && GameVersion == FShaderCache::GetGameVersion() )
		{
			
			Ar << Info.PlatformCaches;
		}
	}
	return Ar;
}

FArchive& operator<<( FArchive& Ar, FShaderCodeCache& Info )
{
	uint32 CacheVersion = Ar.IsLoading() ? (uint32)~0u : ((uint32)FShaderCacheCustomVersion::Latest);
	uint32 GameVersion = Ar.IsLoading() ? (uint32)~0u : ((uint32)FShaderCache::GetGameVersion());
	
	Ar << CacheVersion;
	if ( !Ar.IsError() && CacheVersion == FShaderCacheCustomVersion::Latest )
	{
		Ar << GameVersion;
		
		if ( !Ar.IsError() && GameVersion == FShaderCache::GetGameVersion() )
		{
			Ar << Info.Shaders;
			Ar << Info.Pipelines;
		}
	}
	return Ar;
}

static FORCEINLINE uint32 CalculateSizeOfSamplerStateInitializer()
{
	static uint32 SizeOfSamplerStateInitializer = 0;
	if (SizeOfSamplerStateInitializer == 0)
	{
		TArray<uint8> Data;
		FMemoryWriter Writer(Data);
		FSamplerStateInitializerRHI State;
		Writer << State;
		SizeOfSamplerStateInitializer = Data.Num();
	}
	return SizeOfSamplerStateInitializer;
}

bool FSamplerStateInitializerRHIKeyFuncs::Matches(KeyInitType A,KeyInitType B)
{
	return FMemory::Memcmp(&A, &B, CalculateSizeOfSamplerStateInitializer()) == 0;
}

uint32 FSamplerStateInitializerRHIKeyFuncs::GetKeyHash(KeyInitType Key)
{
	return FCrc::MemCrc_DEPRECATED(&Key, CalculateSizeOfSamplerStateInitializer());
}
