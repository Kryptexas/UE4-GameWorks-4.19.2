// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	HudSettings.h: Declares the UHudSettings class.
=============================================================================*/

#pragma once

#include "PhysicsSettings.generated.h"

USTRUCT()
struct FPhysicalSurfaceName
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TEnumAsByte<enum EPhysicalSurface> Type;

	UPROPERTY()
	FName Name;

	FPhysicalSurfaceName()
		: Type(SurfaceType_Max)
	{}
	FPhysicalSurfaceName(EPhysicalSurface InType, const FName & InName)
		: Type(InType)
		, Name(InName)
	{}
};

UCLASS(config=Engine)
class ENGINE_API UPhysicsSettings: public UObject
{
	GENERATED_UCLASS_BODY()

	/**Max Physics Delta Time to be clamped. */
	UPROPERTY(config, EditAnywhere, meta=(ClampMin="0.0013", UIMin = "0.0013", ClampMax="1.0", UIMax="1.0"), Category=Framerate)
	float MaxPhysicsDeltaTime;

	/** Whether to substep the physics simulation. This feature is still experimental. Certain functionality might not work correctly*/
	UPROPERTY(config, EditAnywhere, Category = Framerate)
	bool bSubstepping;

		/**Max delta time for an individual substep simulation. */
	UPROPERTY(config, EditAnywhere, meta = (ClampMin = "0.0013", UIMin = "0.0013", ClampMax = "1.0", UIMax = "1.0", editcondition = "bSubStepping"), Category=Framerate)
	float MaxSubstepDeltaTime;

	/** Max number of substeps for physics simulation. */
	UPROPERTY(config, EditAnywhere, meta = (ClampMin = "1", UIMin = "1", ClampMax = "16", UIMax = "16", editcondition = "bSubstepping"), Category=Framerate)
	int32 MaxSubsteps;

	/**Physics delta time smoothing factor for sync scene. */
	UPROPERTY(config, EditAnywhere, AdvancedDisplay, meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"), Category = Framerate)
	float SyncSceneSmoothingFactor;

	/**Physics delta time smoothing factor for async scene. */
	UPROPERTY(config, EditAnywhere, AdvancedDisplay, meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "1.0", UIMax = "1.0"), Category = Framerate)
	float AsyncSceneSmoothingFactor;

	/**Physics delta time initial average. */
	UPROPERTY(config, EditAnywhere, AdvancedDisplay, meta = (ClampMin = "0.0013", UIMin = "1.0", ClampMax = "1.0", UIMax = "1.0"), Category = Framerate)
	float InitialAverageFrameRate;

	// PhysicalMaterial Surface Types
	UPROPERTY(config)
	TArray<FPhysicalSurfaceName> PhysicalSurfaces;

public:
	static UPhysicsSettings * Get() { return CastChecked<UPhysicsSettings>(UPhysicsSettings::StaticClass()->GetDefaultObject()); }

	virtual void	PostInitProperties() OVERRIDE;

#if WITH_EDITOR
	virtual bool CanEditChange( const UProperty* Property ) const OVERRIDE;
	/** Load Material Type data from INI file **/
	/** this changes displayname meta data. That means we won't need it outside of editor*/
	void LoadSurfaceType();
#endif // WITH_EDITOR
};