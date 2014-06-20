// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "GameplayCueActor.h"

AGameplayCueActor::AGameplayCueActor(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	AttachToOwner = false;
}

void AGameplayCueActor::BeginPlay()
{
	Super::BeginPlay();

	if (AttachToOwner)
	{
		this->AttachRootComponentToActor(GetOwner(), AttachSocketName, EAttachLocation::SnapToTarget);
	}
}