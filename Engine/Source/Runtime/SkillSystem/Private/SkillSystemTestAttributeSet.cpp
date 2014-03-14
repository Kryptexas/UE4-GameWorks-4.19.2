// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"

USkillSystemTestAttributeSet::USkillSystemTestAttributeSet(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	Health = 100.f;
	Damage = 0.f;
	CritChance = 1.f;
}

