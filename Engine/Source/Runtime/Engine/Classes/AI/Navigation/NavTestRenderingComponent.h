// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NavTestRenderingComponent.generated.h"

UCLASS(hidecategories=Object)
class UNavTestRenderingComponent: public UPrimitiveComponent
{
	GENERATED_BODY()
public:
	UNavTestRenderingComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform &LocalToWorld) const override;
	virtual void CreateRenderState_Concurrent() override;
	virtual void DestroyRenderState_Concurrent() override;
};
