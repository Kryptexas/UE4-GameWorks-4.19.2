// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PaperTileSetThumbnailRenderer.generated.h"

UCLASS()
class UPaperTileSetThumbnailRenderer : public UDefaultSizedThumbnailRenderer
{
	GENERATED_UCLASS_BODY()

	// UThumbnailRenderer interface
	virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget*, FCanvas* Canvas) OVERRIDE;
	// End of UThumbnailRenderer interface
};
