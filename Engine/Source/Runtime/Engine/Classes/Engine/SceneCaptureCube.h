// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * 
 *
 */

#pragma once
#include "SceneCaptureCube.generated.h"

UCLASS(hidecategories = (Collision, Material, Attachment, Actor), HeaderGroup = Decal, MinimalAPI)
class ASceneCaptureCube : public ASceneCapture
{
	GENERATED_UCLASS_BODY()

	/** Scene capture component. */
	UPROPERTY(Category = DecalActor, VisibleAnywhere, BlueprintReadOnly)
	TSubobjectPtr<class USceneCaptureComponentCube> CaptureComponentCube;

	/** To allow drawing the camera frustum in the editor. */
	UPROPERTY()
	TSubobjectPtr<class UDrawFrustumComponent> DrawFrustum;

public:

	// Begin AActor interface
	virtual void PostActorCreated() OVERRIDE;
#if WITH_EDITOR
	ENGINE_API virtual void PostEditMove(bool bFinished) OVERRIDE;
#endif
	// End AActor interface.

	/** Used to synchronize the DrawFrustumComponent with the SceneCaptureComponentCube settings. */
	void UpdateDrawFrustum();

	UFUNCTION(BlueprintCallable, Category = "Rendering")
	void OnInterpToggle(bool bEnable);
};



