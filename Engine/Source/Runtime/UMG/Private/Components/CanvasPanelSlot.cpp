// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UCanvasPanelSlot

UCanvasPanelSlot::UCanvasPanelSlot(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Position = FVector2D::ZeroVector;
	Size = FVector2D(1.0f, 1.0f);
	HorizontalAlignment = HAlign_Left;
	VerticalAlignment = VAlign_Top;
}
