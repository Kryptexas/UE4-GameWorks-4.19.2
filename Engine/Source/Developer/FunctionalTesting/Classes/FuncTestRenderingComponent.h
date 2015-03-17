// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Components/PrimitiveComponent.h"
#include "FuncTestRenderingComponent.generated.h"

UCLASS(hidecategories=Object, editinlinenew)
class UFuncTestRenderingComponent : public UPrimitiveComponent
{
	GENERATED_BODY()
public:
	UFuncTestRenderingComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Begin UPrimitiveComponent Interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual bool ShouldRecreateProxyOnUpdateTransform() const override { return true; }
	// End UPrimitiveComponent Interface

	// Begin USceneComponent Interface
	virtual FBoxSphereBounds CalcBounds(const FTransform &LocalToWorld) const override;
	// End USceneComponent Interface
};
