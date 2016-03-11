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
	
	/** The dockable */
	UPROPERTY()
	class UStaticMeshComponent* SelectionBar;

public:

	/** Default constructor */
	AVREditorDockableWindow();

	/** Updates the selection bar scale and the widgetcomponent position */
	virtual void TickManually( float DeltaTime ) override;

	/** Updates the last dragged relative position */
	void UpdateRelativeRoomTransform();

private:

	/** Scale for the selection bar that will be updated with the worldscale */
	FVector SelectionBarScale;
};	