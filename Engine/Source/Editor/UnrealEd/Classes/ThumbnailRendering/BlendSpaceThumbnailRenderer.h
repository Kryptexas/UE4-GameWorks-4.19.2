// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/**
 *
 * This thumbnail renderer displays a given blend space
 */

#pragma once
#include "BlendSpaceThumbnailRenderer.generated.h"

UCLASS(config=Editor, MinimalAPI)
class UBlendSpaceThumbnailRenderer : public UDefaultSizedThumbnailRenderer
{
	GENERATED_BODY()
public:
	UNREALED_API UBlendSpaceThumbnailRenderer(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin UThumbnailRenderer Object
	UNREALED_API virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget*, FCanvas* Canvas) override;
	// End UThumbnailRenderer Object

	// UObject implementation
	UNREALED_API virtual void BeginDestroy() override;

private:
	class FBlendSpaceThumbnailScene* ThumbnailScene;
};

