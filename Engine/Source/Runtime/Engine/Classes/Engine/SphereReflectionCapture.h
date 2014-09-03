// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "SphereReflectionCapture.generated.h"

class UDrawSphereComponent;

/** 
 *	Actor used to capture the scene for reflection in a sphere shape 
 *	@see https://docs.unrealengine.com/latest/INT/Resources/ContentExamples/Reflections/1_4
 */
UCLASS(hidecategories = (Collision, Attachment, Actor), MinimalAPI)
class ASphereReflectionCapture : public AReflectionCapture
{
	GENERATED_UCLASS_BODY()

	/** Sphere component used to visualize the capture radius */
	UPROPERTY()
	TSubobjectPtr<UDrawSphereComponent> DrawCaptureRadius;

public:

#if WITH_EDITOR
	// Begin AActor interface.
	virtual void EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown) override;
	// End AActor interface.
#endif

};



