// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CableComponentPluginPrivatePCH.h"


ACableActor::ACableActor(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	CableComponent = PCIP.CreateDefaultSubobject<UCableComponent>(this, TEXT("CableComponent0"));
	RootComponent = CableComponent;
}