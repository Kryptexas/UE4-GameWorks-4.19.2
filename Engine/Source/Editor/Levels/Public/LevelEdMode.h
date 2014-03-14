// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LevelEditorViewport.h"

class FEdModeLevel : public FEdMode
{
public:

	/** Constructor */
	FEdModeLevel();

	/** Destructor */
	virtual ~FEdModeLevel();

	// Begin FEdMode

	virtual void Exit() OVERRIDE; 

	virtual void AddReferencedObjects( FReferenceCollector& Collector ) OVERRIDE;

	virtual EAxisList::Type GetWidgetAxisToDraw( FWidget::EWidgetMode InWidgetMode ) const OVERRIDE;

	virtual bool ShouldDrawWidget() const OVERRIDE;

	virtual bool UsesTransformWidget(FWidget::EWidgetMode CheckMode) const OVERRIDE;

	virtual FVector GetWidgetLocation() const OVERRIDE;

	virtual bool AllowWidgetMove() OVERRIDE { return true; }

	virtual bool InputDelta( FLevelEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale ) OVERRIDE;

	virtual void Render( const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI ) OVERRIDE;

	virtual bool StartTracking(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport) OVERRIDE;

	virtual bool EndTracking(FLevelEditorViewportClient* InViewportClient, FViewport* InViewport) OVERRIDE;

	virtual bool IsSnapRotationEnabled() OVERRIDE;

	virtual bool SnapRotatorToGridOverride(FRotator& Rotation) OVERRIDE;

	// End FEdMode

	/** Sets the level we will be transforming */ 
	void SetLevel( ULevelStreaming* Level );

	/** Returns true if this is the current level were editing  */ 
	virtual bool IsEditing( ULevelStreaming* Level );

	/** Calls Apply Post Edit Move on all Actors in the level*/
	void ApplyPostEditMove();

private:

	ULevelStreaming* SelectedLevel;
	FTransform LevelTransform;
	UMaterialInstanceDynamic* BoxMaterial;
	FBox LevelBounds;
	bool bIsTracking;
	bool bIsDirty;
};

