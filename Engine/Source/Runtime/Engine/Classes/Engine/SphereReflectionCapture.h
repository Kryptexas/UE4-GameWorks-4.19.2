// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * 
 *
 */

#pragma once
#include "SphereReflectionCapture.generated.h"

UCLASS(hidecategories=(Collision, Attachment, Actor), HeaderGroup=Decal, MinimalAPI)
class ASphereReflectionCapture : public AReflectionCapture
{
	GENERATED_UCLASS_BODY()

	// Reference to the draw capture radius component
	UPROPERTY()
	TSubobjectPtr<UDrawSphereComponent> DrawCaptureRadius;

public:

#if WITH_EDITOR
	// Begin AActor interface.
	virtual void EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown) OVERRIDE;
	// End AActor interface.
#endif

};



