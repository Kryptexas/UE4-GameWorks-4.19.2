// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Not yet implemented plane capture class
 */

#pragma once
#include "PlaneReflectionCapture.generated.h"

UCLASS(abstract, hidecategories=(Collision, Attachment, Actor), HeaderGroup=Decal, MinimalAPI)
class APlaneReflectionCapture : public AReflectionCapture
{
	GENERATED_UCLASS_BODY()

public:

#if WITH_EDITOR
	// Begin AActor interface.
	virtual void EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown) OVERRIDE;
	// End AActor interface.
#endif

};



