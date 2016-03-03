// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VREditorFloatingUI.h"
#include "VREditorDockableWindow.generated.h"

/**
 * An interactive floating UI panel that can be dragged around
 */
UCLASS()
class AVREditorDockableWindow : public AVREditorFloatingUI
{
	GENERATED_BODY()
	
	/** The dockable window mesh */
	UPROPERTY()
	class UStaticMeshComponent* WindowMeshComponent;

	/** Mesh underneath the window for easy selecting and dragging */
	UPROPERTY()
	class UStaticMeshComponent* SelectionMeshComponent;

public:

	/** Default constructor */
	AVREditorDockableWindow();

	/** Updates the meshes for the UI */
	virtual void TickManually( float DeltaTime ) override;

	/** Updates the last dragged relative position */
	void UpdateRelativeRoomTransform();

	/** Enter hover with laser changes the color of SelectionMesh */
	void OnEnterHover( const FHitResult& HitResult );

	/** Leaving hover with laser changes the color of SelectionMesh */
	void OnLeaveHover();

private:

	/** Setting the color on the dynamic materials of the SelectionMesh */
	void SetSelectionBarColor( const FLinearColor& LinearColor );

	/** The room space scale for the SelectionMeshComponent */
	FVector SelectionMeshScale;

	/** Dynamic material  (opaque) */
	UPROPERTY()
	class UMaterialInstanceDynamic* HoverMaterialMID;

	/** Dynamic material (translucent) */
	UPROPERTY()
	class UMaterialInstanceDynamic* TranslucentHoverMID;
};	