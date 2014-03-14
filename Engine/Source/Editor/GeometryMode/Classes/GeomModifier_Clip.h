// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/**
 * Allows clipping of BSP brushes against a plane.
 */

#pragma once
#include "GeomModifier_Clip.generated.h"

UCLASS()
class UGeomModifier_Clip : public UGeomModifier_Edit
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Settings)
	uint32 bFlipNormal:1;

	UPROPERTY(EditAnywhere, Category=Settings)
	uint32 bSplit:1;

	/** The clip markers that the user has dropped down in the world so far. */
	UPROPERTY()
	TArray<FVector> ClipMarkers;

	/** The mouse position, in world space, where the user currently is hovering. */
	UPROPERTY()
	FVector SnappedMouseWorldSpacePos;


	// Begin UGeomModifier Interface
	virtual bool Supports() OVERRIDE;
	virtual bool InputKey(class FLevelEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) OVERRIDE;
	virtual void Render(const FSceneView* View,FViewport* Viewport,FPrimitiveDrawInterface* PDI) OVERRIDE;
	virtual void DrawHUD(FLevelEditorViewportClient* ViewportClient,FViewport* Viewport,const FSceneView* View,FCanvas* Canvas) OVERRIDE;
	virtual void Tick(FLevelEditorViewportClient* ViewportClient,float DeltaTime) OVERRIDE;
	virtual void WasActivated() OVERRIDE;
protected:
	virtual bool OnApply() OVERRIDE;
	// End UGeomModifier Interface

private:
	// @todo document
	void ApplyClip( bool InSplit, bool InFlipNormal );
};



