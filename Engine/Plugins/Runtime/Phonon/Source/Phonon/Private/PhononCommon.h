//
// Copyright (C) Impulsonic, Inc. All rights reserved.
//

#pragma once

#include <phonon.h>
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(LogPhonon, Log, All);

struct SimulationQualitySettings
{
	int32 Bounces;
	int32 Rays;
	int32 SecondaryRays;
};

UENUM(BlueprintType)
enum class EQualitySettings : uint8
{
	LOW				UMETA(DisplayName = "Low"),
	MEDIUM			UMETA(DisplayName = "Medium"),
	HIGH			UMETA(DisplayName = "High"),
	CUSTOM			UMETA(DisplayName = "Custom")
};

extern TMap<EQualitySettings, SimulationQualitySettings> RealtimeSimulationQualityPresets;
extern TMap<EQualitySettings, SimulationQualitySettings> BakedSimulationQualityPresets;

UENUM(BlueprintType)
enum class EIplSpatializationMethod : uint8
{
	// Classic 2D panning - fast.
	PANNING			UMETA(DisplayName = "Panning"),
	// Full 3D audio processing with HRTF.
	HRTF			UMETA(DisplayName = "HRTF")
};

UENUM(BlueprintType)
enum class EIplHrtfInterpolationMethod : uint8
{
	// Uses a nearest neighbor lookup - fast.
	NEAREST			UMETA(DisplayName = "Nearest"),
	// Bilinearly interpolates the HRTF before processing. Slower, but can result in a smoother sound as the listener rotates.
	BILINEAR		UMETA(DisplayName = "Bilinear")
};

UENUM(BlueprintType)
enum class EIplDirectOcclusionMethod : uint8
{
	// Do not perform any occlusion test.
	NONE			UMETA(DisplayName = "None"),
	// Binary visible or not test. Adjusts direct volume accordingly.
	RAYCAST			UMETA(DisplayName = "Raycast"),
	// Treats the source as a sphere instead of a point. Smoothly ramps up volume as source becomes visible to listener.
	VOLUMETRIC		UMETA(DisplayName = "Partial")
};

UENUM(BlueprintType)
enum class EIplSimulationType : uint8
{
	// Simulate indirect sound at run time.
	REALTIME		UMETA(DisplayName = "Real-Time"),
	// Precompute indirect sound.
	BAKED			UMETA(DisplayName = "Baked"),
	DISABLED		UMETA(DisplayName = "Disabled")
};

UENUM(BlueprintType)
enum class EIplIndirectSpatializationMethod : uint8
{
	PANNING				UMETA(DisplayName = "Panning"),
	HRTF				UMETA(DisplayName = "HRTF")
};

UENUM(BlueprintType)
enum class EIplAudioEngine : uint8
{
	UNREAL			UMETA(DisplayName = "Unreal"),
	FMOD			UMETA(DisplayName = "FMOD"),
	WWISE			UMETA(DisplayName = "Wwise")
};

namespace Phonon
{
	extern const IPLContext PHONON_API GlobalContext;

	IPLVector3 PHONON_API IPLVector3FromFVector(const FVector& Coords);
	FVector PHONON_API FVectorFromIPLVector3(const IPLVector3& Coords);

	FVector PHONON_API UnrealToPhononFVector(const FVector& Coords, const bool bScale = true);
	IPLVector3 PHONON_API UnrealToPhononIPLVector3(const FVector& Coords, const bool bScale = true);
	FVector PHONON_API PhononToUnrealFVector(const FVector& Coords, const bool bScale = true);
	IPLVector3 PHONON_API PhononToUnrealIPLVector3(const FVector& Coords, const bool bScale = true);

	void LogPhononStatus(IPLerror Status);

	void* LoadDll(const FString& DllFile);
	FString ErrorCodeToFString(const IPLerror ErrorCode);
}

