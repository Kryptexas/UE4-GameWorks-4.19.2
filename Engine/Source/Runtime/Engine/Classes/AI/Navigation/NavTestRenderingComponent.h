// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NavTestRenderingComponent.generated.h"

UCLASS(HeaderGroup=Component, hidecategories=Object)
class UNavTestRenderingComponent: public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	virtual FBoxSphereBounds CalcBounds(const FTransform &LocalToWorld) const OVERRIDE;
	virtual void CreateRenderState_Concurrent() OVERRIDE;
	virtual void DestroyRenderState_Concurrent() OVERRIDE;
};
