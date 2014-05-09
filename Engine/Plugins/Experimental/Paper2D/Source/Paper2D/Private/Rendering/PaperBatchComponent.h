// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperBatchComponent.generated.h"

// Dummy component designed to create a batch manager render scene proxy that does work on the render thread
UCLASS()
class UPaperBatchComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	// UPrimitiveComponent interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const OVERRIDE;
	// End of UPrimitiveComponent interface
};
