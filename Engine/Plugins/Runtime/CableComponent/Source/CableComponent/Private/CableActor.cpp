// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CableComponentPluginPrivatePCH.h"


ACableActor::ACableActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	CableComponent = ObjectInitializer.CreateDefaultSubobject<UCableComponent>(this, TEXT("CableComponent0"));
	RootComponent = CableComponent;
}