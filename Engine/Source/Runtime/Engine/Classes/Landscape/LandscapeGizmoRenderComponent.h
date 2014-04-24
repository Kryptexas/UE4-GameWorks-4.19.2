// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "LandscapeGizmoRenderComponent.generated.h"

UCLASS(HeaderGroup=Terrain, hidecategories=Object)
class ULandscapeGizmoRenderComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()


	// Begin UPrimitiveComponent Interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	// End UPrimitiveComponent Interface

	// Begin USceneComponent interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const OVERRIDE;
	// End USceneComponent interface.
};



