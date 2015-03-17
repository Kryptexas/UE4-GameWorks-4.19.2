// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "LandscapeGizmoRenderComponent.generated.h"

UCLASS(hidecategories=Object)
class ULandscapeGizmoRenderComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
public:
	ULandscapeGizmoRenderComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin UPrimitiveComponent Interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	// End UPrimitiveComponent Interface

	// Begin USceneComponent interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	// End USceneComponent interface.
};



