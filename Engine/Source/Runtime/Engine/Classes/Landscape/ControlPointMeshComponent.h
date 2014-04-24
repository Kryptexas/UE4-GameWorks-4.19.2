// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ControlPointMeshComponent.generated.h"

UCLASS(HeaderGroup=Terrain, MinimalAPI)
class UControlPointMeshComponent : public UStaticMeshComponent
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITORONLY_DATA
	UPROPERTY(transient)
	uint32 bSelected:1;
#endif

	//Begin UPrimitiveComponent Interface
#if WITH_EDITOR
	virtual bool ShouldRenderSelected() const OVERRIDE
	{
		return Super::ShouldRenderSelected() || bSelected;
	}
#endif
	//End UPrimitiveComponent Interface
};
