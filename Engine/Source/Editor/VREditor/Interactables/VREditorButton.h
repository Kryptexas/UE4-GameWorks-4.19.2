// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "VREditorBaseActor.h"
#include "ViewportInteractableInterface.h"
#include "VREditorButton.generated.h"

//Forward declarations
class UViewportInteractor;

/**
* A button for VR Editor
*/
UCLASS()
class AVREditorButton : public AVREditorBaseActor, public IViewportInteractableInterface
{
	GENERATED_BODY()

public:

	/** Default constructor which sets up safe defaults */
	AVREditorButton();

	// Begin IViewportInteractableInterface
	virtual void OnPressed( UViewportInteractor* Interactor, const FHitResult& InHitResult, bool& bOutResultedInDrag  ) override;
	virtual void OnHover( UViewportInteractor* Interactor ) override;
	virtual void OnHoverEnter( UViewportInteractor* Interactor, const FHitResult& InHitResult ) override;
	virtual void OnHoverLeave( UViewportInteractor* Interactor, const UActorComponent* NewComponent ) override;
	virtual void OnDrag( UViewportInteractor* Interactor ) override;
	virtual void OnDragRelease( UViewportInteractor* Interactor ) override;
	virtual class UViewportDragOperationComponent* GetDragOperationComponent() override { return nullptr; };
	// End IViewportInteractableInterface

private:

	/** Mesh for the button to interact with */
	UPROPERTY()
	UStaticMeshComponent* ButtonMeshComponent;

};