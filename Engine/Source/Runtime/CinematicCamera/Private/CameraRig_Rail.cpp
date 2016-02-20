// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "CinematicCameraPrivate.h"
#include "CameraRig_Rail.h"

#define LOCTEXT_NAMESPACE "CameraRig_Rail"

// #TODO WRITEME

ACameraRig_Rail::ACameraRig_Rail(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

#if WITH_EDITOR
void ACameraRig_Rail::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

USceneComponent* ACameraRig_Rail::GetDefaultAttachComponent() const
{
	return CraneRailMount;
}
#endif



#undef LOCTEXT_NAMESPACE