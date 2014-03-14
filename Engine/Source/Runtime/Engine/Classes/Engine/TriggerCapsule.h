// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "TriggerCapsule.generated.h"

UCLASS(MinimalAPI)
class ATriggerCapsule : public ATriggerBase
{
	GENERATED_UCLASS_BODY()


#if WITH_EDITOR
	// Begin AActor interface.
	virtual void EditorApplyScale(const FVector& DeltaScale, const FVector* PivotLocation, bool bAltDown, bool bShiftDown, bool bCtrlDown) OVERRIDE;
	// End AActor interface.
#endif
};



