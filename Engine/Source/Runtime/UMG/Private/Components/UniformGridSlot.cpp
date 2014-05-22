// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UMGPrivatePCH.h"

/////////////////////////////////////////////////////
// UUniformGridSlot

UUniformGridSlot::UUniformGridSlot(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	HorizontalAlignment = HAlign_Left;
	VerticalAlignment = VAlign_Top;
}
