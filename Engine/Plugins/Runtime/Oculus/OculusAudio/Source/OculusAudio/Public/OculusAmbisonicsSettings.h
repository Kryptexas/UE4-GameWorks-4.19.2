// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IAudioExtensionPlugin.h"

#include "OculusAmbisonicsSettings.generated.h"


UENUM()
enum class EAmbisonicMode : uint8
{
	// High quality ambisonic spatialization method
	SphericalHarmonics,

	// Alternative ambisonic spatialization method
	VirtualSpeakers,
};

UENUM()
enum class EAmbisonicFormat : uint8
{
	// Standard B-Format, WXYZ
	FuMa UMETA(DisplayName = "FuMa (B-Format, WXYZ)"),

	// ACN/SN3D Standard, WYZX
	AmbiX UMETA(DisplayName = "AmbiX (ACN/SN3D, WYZX)"),
};

USTRUCT(BlueprintType)
struct OCULUSAUDIO_API FSubmixEffectOculusAmbisonicSpatializerSettings
{
	GENERATED_USTRUCT_BODY()

		// Ambisonic spatialization mode
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Realtime)
	EAmbisonicMode AmbisonicMode;

	FSubmixEffectOculusAmbisonicSpatializerSettings()
		: AmbisonicMode(EAmbisonicMode::SphericalHarmonics)
	{
	}
};

UCLASS()
class OCULUSAUDIO_API UOculusAmbisonicsSettings : public UAmbisonicsSubmixSettingsBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Ambisonics)
	EAmbisonicMode SpatializationMode;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Ambisonics)
	EAmbisonicFormat ChannelOrder;

};

