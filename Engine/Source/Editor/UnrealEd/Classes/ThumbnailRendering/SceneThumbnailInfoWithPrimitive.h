// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Thumbnail information for assets that need a scene and a primitive
 */

#pragma once
#include "SceneThumbnailInfoWithPrimitive.generated.h"


UCLASS(dependson=UThumbnailManager,MinimalAPI)
class USceneThumbnailInfoWithPrimitive : public USceneThumbnailInfo
{
	GENERATED_UCLASS_BODY()

	/** The type of primitive used in this thumbnail */
	UPROPERTY()
	TEnumAsByte<EThumbnailPrimType> PrimitiveType;

	/** The custom mesh used when the primitive type is TPT_None */
	UPROPERTY()
	FStringAssetReference PreviewMesh;

	virtual void PostLoad() OVERRIDE;
};
