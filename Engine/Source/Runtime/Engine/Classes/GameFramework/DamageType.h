// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/**
 * DamageType, the base class of all damage types.
 * These are never instanced, just used as static code and data holders.
 */

#pragma once
#include "DamageType.generated.h"

UCLASS(MinimalAPI, const, Blueprintable, BlueprintType)
class UDamageType : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=DamageType)
	uint32 bCausedByWorld:1;    //this damage was caused by the world (falling off level, into lava, etc)

	/** Whether to scale imparted momentum by the receiving pawn's mass for pawns using character movement */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=DamageType)
	uint32 bScaleMomentumByMass:1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=RigidBody)
	float DamageImpulse;    // magnitude of impulse applied to  AActor  due to this damage type.

	/** When applying radial impulses, whether to treat as impulse or velocity change. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=RigidBody)
	uint32 bRadialDamageVelChange:1;

	/** How large the impulse should be applied to destructible meshes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Destruction)
	float DestructibleImpulse;

	/** How much the damage spreads on a destructible mesh */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Destruction)
	float DestructibleDamageSpreadScale;

	/** Damage fall-off for radius damage (exponent).  Default 1.0=linear, 2.0=square of distance, etc. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=DamageType)
	float DamageFalloff;

};



