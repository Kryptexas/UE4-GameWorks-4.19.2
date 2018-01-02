// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "PrimitiveStats.h"
#include "UObject/CoreNet.h"

UPrimitiveStats::UPrimitiveStats(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UPrimitiveStats::UpdateNames()
{
	if( Object.IsValid() )
	{
		Type = Object.Get()->GetClass()->GetName();
	}
}
