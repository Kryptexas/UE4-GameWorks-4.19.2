// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"

//////////////////////////////////////////////////////////////////////////
// UPaperTerrainSplineComponent

UPaperTerrainSplineComponent::UPaperTerrainSplineComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

#if WITH_EDITORONLY_DATA
void UPaperTerrainSplineComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	OnSplineEdited.ExecuteIfBound();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif
