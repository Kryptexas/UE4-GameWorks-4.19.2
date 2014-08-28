// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "GameplayAbilityTargetTypes.h"


bool FGameplayAbilityTargetData_Radius::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << Actors;	// Fixme: will this go through the package map properly?
	Ar << Origin;

	return true;
}