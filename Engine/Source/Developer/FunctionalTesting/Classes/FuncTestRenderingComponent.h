// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "FuncTestRenderingComponent.generated.h"

UCLASS(hidecategories=Object, editinlinenew)
class UFuncTestRenderingComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	// Begin UPrimitiveComponent Interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	// End UPrimitiveComponent Interface

	// Begin USceneComponent Interface
	virtual FBoxSphereBounds CalcBounds(const FTransform &LocalToWorld) const OVERRIDE;
	// End USceneComponent Interface
};