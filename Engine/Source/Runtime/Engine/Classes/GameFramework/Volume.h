// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// Volume:  a bounding volume
//=============================================================================

#pragma once
#include "Volume.generated.h"

UCLASS(MinimalAPI, showcategories=Collision, hidecategories=(Brush, Physics), abstract)
class AVolume : public ABrush
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	/** Delegate used for notifications when a volumes initial shape changes */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnVolumeShapeChanged, AVolume& )

	/** Function to get the 'Volume imported' delegate */
	static ENGINE_API FOnVolumeShapeChanged& GetOnVolumeShapeChangedDelegate()
	{
		return OnVolumeShapeChanged;
	}
private:
	/** Called during posteditchange after the volume's initial shape has changed */
	static FOnVolumeShapeChanged OnVolumeShapeChanged;
public:
	// Begin AActor Interface
	/**
	* Function that gets called from within Map_Check to allow this actor to check itself
	* for any potential errors and register them with map check dialog.
	*/
	ENGINE_API virtual void CheckForErrors() OVERRIDE;
#endif // WITH_EDITOR
	
	ENGINE_API virtual bool IsLevelBoundsRelevant() const OVERRIDE;
	// End AActor Interface

	// Begin Brush Interface
	ENGINE_API virtual bool IsStaticBrush() const OVERRIDE;
	ENGINE_API virtual bool IsVolumeBrush() const OVERRIDE;
	// End Brush Interface

	/** @returns true if a sphere/point (with optional radius CheckRadius) overlaps this volume */
	ENGINE_API bool EncompassesPoint(FVector Point, float SphereRadius=0.f, float* OutDistanceToPoint = 0);

	//Begin UObject Interface
#if WITH_EDITOR
	ENGINE_API virtual void PostEditImport() OVERRIDE;
	ENGINE_API virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	//End UObject Interface


};



