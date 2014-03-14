// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UDamageType::UDamageType(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	DamageImpulse = 800.0f;
	DestructibleImpulse = 800.0f;
	DestructibleDamageSpreadScale = 1.0f;
	bScaleMomentumByMass = true;
	DamageFalloff = 1.0f;
}
