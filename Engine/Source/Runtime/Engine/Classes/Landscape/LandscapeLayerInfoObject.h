// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LandscapeLayerInfoObject.generated.h"

UCLASS(HeaderGroup=Terrain, MinimalAPI)
class ULandscapeLayerInfoObject : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(VisibleAnywhere, Category=LandscapeLayerInfoObject)
	FName LayerName;

	UPROPERTY(EditAnywhere, Category=LandscapeLayerInfoObject)
	class UPhysicalMaterial* PhysMaterial;

	UPROPERTY(EditAnywhere, Category=LandscapeLayerInfoObject)
	float Hardness;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category=LandscapeLayerInfoObject)
	uint32 bNoWeightBlend:1;
#endif // WITH_EDITORONLY_DATA

#if WITH_EDITOR
	// Begin UObject Interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	virtual void PostLoad() OVERRIDE;
	// End UObject Interface
#endif
};



