// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "AI/Navigation/NavAreas/NavArea_LowHeight.h"

UNavArea_LowHeight::UNavArea_LowHeight(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	DefaultCost = BIG_NUMBER;
	DrawColor = FColor(0, 0, 128);

	// can't traverse
	AreaFlags = 0;
}
