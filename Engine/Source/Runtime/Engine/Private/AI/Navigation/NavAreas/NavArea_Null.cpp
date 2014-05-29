// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "AI/Navigation/NavAreas/NavArea_Null.h"

UNavArea_Null::UNavArea_Null(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	DefaultCost = BIG_NUMBER;
}
