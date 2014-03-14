// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 *
 * Used to affect reverb settings in the game and editor.
 */

#pragma once
#include "ReverbVolume.generated.h"

/**
 * Indicates a reverb preset to use.
 */
UENUM()
enum ReverbPreset
{
	REVERB_Default,
	REVERB_Bathroom,
	REVERB_StoneRoom,
	REVERB_Auditorium,
	REVERB_ConcertHall,
	REVERB_Cave,
	REVERB_Hallway,
	REVERB_StoneCorridor,
	REVERB_Alley,
	REVERB_Forest,
	REVERB_City,
	REVERB_Mountains,
	REVERB_Quarry,
	REVERB_Plain,
	REVERB_ParkingLot,
	REVERB_SewerPipe,
	REVERB_Underwater,
	REVERB_SmallRoom,
	REVERB_MediumRoom,
	REVERB_LargeRoom,
	REVERB_MediumHall,
	REVERB_LargeHall,
	REVERB_Plate,
	REVERB_MAX,
};

/** Struct encapsulating settings for reverb effects. */
USTRUCT()
struct FReverbSettings
{
	GENERATED_USTRUCT_BODY()

	/* Whether to apply the reverb settings below. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ReverbSettings )
	uint32 bApplyReverb:1;

	/** The reverb preset to employ. */
	UPROPERTY()
	TEnumAsByte<enum ReverbPreset> ReverbType_DEPRECATED;

	/** The reverb asset to employ. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ReverbSettings)
	class UReverbEffect* ReverbEffect;

	/** Volume level of the reverb affect. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ReverbSettings)
	float Volume;

	/** Time to fade from the current reverb settings into this setting, in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ReverbSettings)
	float FadeTime;



		FReverbSettings()
		: bApplyReverb(true)
		, ReverbType_DEPRECATED(REVERB_Default)
		, ReverbEffect(NULL)
		, Volume(0.5f)
		, FadeTime(2.0f)
		{
		}

		void PostSerialize(const FArchive& Ar);
	
};

template<>
struct TStructOpsTypeTraits<FReverbSettings> : public TStructOpsTypeTraitsBase
{
	enum 
	{
		WithPostSerialize = true,
	};
};

/** Struct encapsulating settings for interior areas. */
//@warning: manually mirrored in Components.h
USTRUCT()
struct FInteriorSettings
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	uint32 bIsWorldSettings:1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InteriorSettings)
	float ExteriorVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InteriorSettings)
	float ExteriorTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InteriorSettings)
	float ExteriorLPF;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InteriorSettings)
	float ExteriorLPFTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InteriorSettings)
	float InteriorVolume;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InteriorSettings)
	float InteriorTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InteriorSettings)
	float InteriorLPF;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=InteriorSettings)
	float InteriorLPFTime;



		FInteriorSettings()
		: bIsWorldSettings(false)
		, ExteriorVolume(1.0f)
		, ExteriorTime(0.5f)
		, ExteriorLPF(1.0f)
		, ExteriorLPFTime(0.5f)
		, InteriorVolume(1.0f)
		, InteriorTime(0.5f)
		, InteriorLPF(1.0f)
		, InteriorLPFTime(0.5f)
		{
		}
	
};

UCLASS(hidecategories=(Advanced, Attachment, Collision, Volume))
class AReverbVolume : public AVolume
{
	GENERATED_UCLASS_BODY()

	/**
	 * Priority of this volume. In the case of overlapping volumes the one with the highest priority
	 * is chosen. The order is undefined if two or more overlapping volumes have the same priority.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ReverbVolume)
	float Priority;

	/** whether this volume is currently enabled and able to affect sounds */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, replicated, Category=Toggle)
	uint32 bEnabled:1;

	/** Reverb settings to use for this volume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ReverbVolume)
	struct FReverbSettings Settings;

	/** Interior settings used for this volume */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=ReverbVolume)
	struct FInteriorSettings AmbientZoneSettings;

	/** Next volume in linked listed, sorted by priority in descending order. */
	UPROPERTY(transient)
	class AReverbVolume* NextLowerPriorityVolume;



	// Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	// End UObject Interface

	// Begin AActor Interface
	virtual void PostUnregisterAllComponents() OVERRIDE;
protected:
	virtual void PostRegisterAllComponents() OVERRIDE;
public:
	// End AActor Interface
};



