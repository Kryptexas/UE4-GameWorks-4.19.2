// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * 
 *
 */

#pragma once
#include "SceneCaptureCube.generated.h"

UCLASS(hidecategories = (Collision, Material, Attachment, Actor), MinimalAPI)
class ASceneCaptureCube : public ASceneCapture
{
	GENERATED_UCLASS_BODY()

private:
	/** Scene capture component. */
	UPROPERTY(Category = DecalActor, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	class USceneCaptureComponentCube* CaptureComponentCube;

	/** To allow drawing the camera frustum in the editor. */
	UPROPERTY()
	class UDrawFrustumComponent* DrawFrustum;

public:

	// Begin AActor interface
	virtual void PostActorCreated() override;
#if WITH_EDITOR
	ENGINE_API virtual void PostEditMove(bool bFinished) override;
#endif
	// End AActor interface.

	/** Used to synchronize the DrawFrustumComponent with the SceneCaptureComponentCube settings. */
	void UpdateDrawFrustum();

	UFUNCTION(BlueprintCallable, Category = "Rendering")
	void OnInterpToggle(bool bEnable);

	/** Returns CaptureComponentCube subobject **/
	FORCEINLINE class USceneCaptureComponentCube* GetCaptureComponentCube() const { return CaptureComponentCube; }
	/** Returns DrawFrustum subobject **/
	FORCEINLINE class UDrawFrustumComponent* GetDrawFrustum() const { return DrawFrustum; }
};



