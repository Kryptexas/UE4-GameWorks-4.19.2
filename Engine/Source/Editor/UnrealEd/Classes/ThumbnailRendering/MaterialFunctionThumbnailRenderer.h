// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/**
 *
 * This thumbnail renderer displays a given material function
 */

#pragma once
#include "MaterialFunctionThumbnailRenderer.generated.h"

UCLASS(config=Editor)
class UMaterialFunctionThumbnailRenderer : public UDefaultSizedThumbnailRenderer
{
	GENERATED_BODY()
public:
	UMaterialFunctionThumbnailRenderer(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin UThumbnailRenderer Object
	virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget*, FCanvas* Canvas) override;
	// End UThumbnailRenderer Object

	// UObject implementation
	UNREALED_API virtual void BeginDestroy() override;

private:
	class FMaterialThumbnailScene* ThumbnailScene;
};

