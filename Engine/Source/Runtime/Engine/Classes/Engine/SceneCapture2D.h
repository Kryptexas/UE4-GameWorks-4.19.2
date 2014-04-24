// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * 
 *
 */

#pragma once
#include "SceneCapture2D.generated.h"

UCLASS(hidecategories=(Collision, Material, Attachment, Actor), HeaderGroup=Decal, MinimalAPI)
class ASceneCapture2D : public ASceneCapture
{
	GENERATED_UCLASS_BODY()

	/** Scene capture component. */
	UPROPERTY(Category=DecalActor, VisibleAnywhere, BlueprintReadOnly)
	TSubobjectPtr<class USceneCaptureComponent2D> CaptureComponent2D;

	/** To allow drawing the camera frustum in the editor. */
	UPROPERTY()
	TSubobjectPtr<class UDrawFrustumComponent> DrawFrustum;

public:

	// Begin AActor interface
	ENGINE_API virtual void PostActorCreated() OVERRIDE;
	// End AActor interface.

	/** Used to synchronize the DrawFrustumComponent with the SceneCaptureComponent2D settings. */
	void UpdateDrawFrustum();

	UFUNCTION(BlueprintCallable, Category="Rendering")
	void OnInterpToggle(bool bEnable);
};



