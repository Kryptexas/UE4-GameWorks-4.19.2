// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NavLinkRenderingComponent.generated.h"

UCLASS(HeaderGroup=Component, hidecategories=Object, editinlinenew)
class ENGINE_API UNavLinkRenderingComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()
		
	// Begin UPrimitiveComponent Interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;

	/** Should recreate proxy one very update */
	virtual bool ShouldRecreateProxyOnUpdateTransform() const OVERRIDE { return true; }
	// End UPrimitiveComponent Interface

	// Begin USceneComponent Interface
	virtual FBoxSphereBounds CalcBounds(const FTransform &LocalToWorld) const OVERRIDE;
	// End USceneComponent Interface
};

