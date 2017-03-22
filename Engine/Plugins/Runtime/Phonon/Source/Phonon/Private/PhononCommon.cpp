//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#include "PhononCommon.h"

#include "EngineUtils.h"
#include "FileManager.h"
#include "IPluginManager.h"
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"

DEFINE_LOG_CATEGORY(LogPhonon);

static TMap<EQualitySettings, SimulationQualitySettings> GetRealtimeQualityPresets()
{
	TMap<EQualitySettings, SimulationQualitySettings> Presets;

	SimulationQualitySettings Settings;

	Settings.Bounces = 2;
	Settings.Rays = 4096;
	Settings.SecondaryRays = 512;
	Presets.Add(EQualitySettings::LOW, Settings);

	Settings.Bounces = 4;
	Settings.Rays = 8192;
	Settings.SecondaryRays = 1024;
	Presets.Add(EQualitySettings::MEDIUM, Settings);

	Settings.Bounces = 8;
	Settings.Rays = 16384;
	Settings.SecondaryRays = 2048;
	Presets.Add(EQualitySettings::HIGH, Settings);

	Settings.Bounces = 0;
	Settings.Rays = 0;
	Settings.SecondaryRays = 0;
	Presets.Add(EQualitySettings::CUSTOM, Settings);

	return Presets;
}

static TMap<EQualitySettings, SimulationQualitySettings> GetBakedQualityPresets()
{
	TMap<EQualitySettings, SimulationQualitySettings> Presets;

	SimulationQualitySettings Settings;

	Settings.Bounces = 128;
	Settings.Rays = 16384;
	Settings.SecondaryRays = 2048;
	Presets.Add(EQualitySettings::LOW, Settings);

	Settings.Bounces = 256;
	Settings.Rays = 32768;
	Settings.SecondaryRays = 4096;
	Presets.Add(EQualitySettings::MEDIUM, Settings);

	Settings.Bounces = 512;
	Settings.Rays = 65536;
	Settings.SecondaryRays = 4096;
	Presets.Add(EQualitySettings::HIGH, Settings);

	Settings.Bounces = 0;
	Settings.Rays = 0;
	Settings.SecondaryRays = 0;
	Presets.Add(EQualitySettings::CUSTOM, Settings);

	return Presets;
}

TMap<EQualitySettings, SimulationQualitySettings> RealtimeSimulationQualityPresets = GetRealtimeQualityPresets();
TMap<EQualitySettings, SimulationQualitySettings> BakedSimulationQualityPresets = GetBakedQualityPresets();

namespace Phonon
{
	static void* UnrealAlloc(const size_t size, const size_t alignment)
	{
		return FMemory::Malloc(size, alignment);
	}

	static void UnrealFree(void* ptr)
	{
		FMemory::Free(ptr);
	}

	static void UnrealLog(char* msg)
	{
		FString Message(msg);
		UE_LOG(LogPhonon, Log, TEXT("%s"), *Message); 
	}

	const IPLContext GlobalContext =
	{
		UnrealLog,
		nullptr, //UnrealAlloc, 
		nullptr  //UnrealFree
	};
	 
	// 1 Unreal Unit = 1cm, 1 Phonon Unit = 1m
	static const float PHONON_SCALEFACTOR = 0.01f;

	FVector UnrealToPhononFVector(const FVector& UnrealCoords, const bool bScale)
	{
		FVector PhononCoords;
		PhononCoords.X = UnrealCoords.Y;
		PhononCoords.Y = UnrealCoords.Z; 
		PhononCoords.Z = -UnrealCoords.X; 
		return bScale ? PhononCoords * PHONON_SCALEFACTOR : PhononCoords;
	}

	IPLVector3 UnrealToPhononIPLVector3(const FVector& UnrealCoords, const bool bScale)
	{
		IPLVector3 PhononCoords;
		PhononCoords.x = UnrealCoords.Y;
		PhononCoords.y = UnrealCoords.Z;
		PhononCoords.z = -UnrealCoords.X; 
		
		if (bScale)
		{
			PhononCoords.x *= PHONON_SCALEFACTOR;
			PhononCoords.y *= PHONON_SCALEFACTOR;
			PhononCoords.z *= PHONON_SCALEFACTOR;
		}

		return PhononCoords;
	}

	FVector PhononToUnrealFVector(const FVector& coords, const bool bScale)
	{
		FVector UnrealCoords;
		UnrealCoords.X = -coords.Z;
		UnrealCoords.Y = coords.X;
		UnrealCoords.Z = coords.Y;
		
		if (bScale)
		{
			UnrealCoords.X /= PHONON_SCALEFACTOR;
			UnrealCoords.Y /= PHONON_SCALEFACTOR;
			UnrealCoords.Z /= PHONON_SCALEFACTOR;
		}

		return UnrealCoords;
	}

	IPLVector3 PhononToUnrealIPLVector3(const FVector& coords, const bool bScale)
	{
		IPLVector3 UnrealCoords = {0, 0, 0};

		// TODO

		return UnrealCoords;
	}

	IPLVector3 IPLVector3FromFVector(const FVector& Coords)
	{
		IPLVector3 IplVector;
		IplVector.x = Coords.X;
		IplVector.y = Coords.Y;
		IplVector.z = Coords.Z;
		return IplVector;
	}

	FVector FVectorFromIPLVector3(const IPLVector3& Coords)
	{
		FVector Vector;
		Vector.X = Coords.x;
		Vector.Y = Coords.y;
		Vector.Z = Coords.z;
		return Vector;
	}

	void LogPhononStatus(IPLerror Status)
	{
		if (Status != IPL_STATUS_SUCCESS)
		{
			UE_LOG(LogPhonon, Error, TEXT("Error: %s"), *ErrorCodeToFString(Status));
		}
	}

	void* LoadDll(const FString& DllFile)
	{
		UE_LOG(LogPhonon, Log, TEXT("Attempting to load %s"), *DllFile);

		void* DllHandle = nullptr;

		if (FPaths::FileExists(DllFile))
		{
			DllHandle = FPlatformProcess::GetDllHandle(*DllFile);
		}
		else
		{
			UE_LOG(LogPhonon, Error, TEXT("File does not exist. %s"), *DllFile);
		}

		if (!DllHandle)
		{
			UE_LOG(LogPhonon, Error, TEXT("Unable to load %s."), *DllFile);
		}
		else
		{
			UE_LOG(LogPhonon, Log, TEXT("Loaded %s."), *DllFile);
		}

		return DllHandle;
	}

	/**
	* Returns a string representing the given error code.
	*/
	FString ErrorCodeToFString(const IPLerror ErrorCode)
	{
		switch (ErrorCode)
		{
		case IPL_STATUS_SUCCESS:
			return FString(TEXT("Success."));
		case IPL_STATUS_FAILURE:
			return FString(TEXT("Failure."));
		case IPL_STATUS_OUTOFMEMORY:
			return FString(TEXT("Out of memory."));
		case IPL_STATUS_INITIALIZATION:
			return FString(TEXT("Initialization error."));
		default:
			return FString(TEXT("Unknown error."));
		}
	}
}
