// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 *
 * This thumbnail renderer displays a given static mesh
 */

#pragma once
#include "BlueprintThumbnailRenderer.generated.h"

UCLASS(config=Editor,MinimalAPI)
class UBlueprintThumbnailRenderer : public UDefaultSizedThumbnailRenderer
{
	GENERATED_UCLASS_BODY()


	// Begin UThumbnailRenderer Object
	UNREALED_API virtual void Draw(UObject* Object, int32 X, int32 Y, uint32 Width, uint32 Height, FRenderTarget*, FCanvas* Canvas) OVERRIDE;
	// End UThumbnailRenderer Object

	// UObject implementation
	UNREALED_API virtual void BeginDestroy() OVERRIDE;
	// End UObject implementation

	/** Returns true if this scene is capable of creating a thumbnail with the specified blueprint */
	bool CanVisualizeBlueprint(class UBlueprint* Blueprint);

	/** Notifies the thumbnail scene to refresh components for the specified blueprint */
	void BlueprintChanged(class UBlueprint* Blueprint);

private:
	class FBlueprintThumbnailScene* ThumbnailScene;
};

