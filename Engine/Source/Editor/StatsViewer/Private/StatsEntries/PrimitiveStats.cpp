// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "StatsViewerPrivatePCH.h"
#include "PrimitiveStats.h"

UPrimitiveStats::UPrimitiveStats(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UPrimitiveStats::UpdateNames()
{
	if( Object.IsValid() )
	{
		Type = Object.Get()->GetClass()->GetName();
	}
}