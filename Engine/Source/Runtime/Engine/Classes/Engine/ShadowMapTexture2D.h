// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ShadowMapTexture2D.generated.h"

UCLASS(MinimalAPI)
class UShadowMapTexture2D : public UTexture2D
{
	GENERATED_UCLASS_BODY()
	
	/** Bit-field with shadowmap flags. */
	UPROPERTY()
	TEnumAsByte<enum EShadowMapFlags> ShadowmapFlags;
};
