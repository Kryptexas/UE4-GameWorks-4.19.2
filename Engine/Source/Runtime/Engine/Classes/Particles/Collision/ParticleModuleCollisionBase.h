// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ParticleModuleCollisionBase.generated.h"

/**
 *	Flags indicating what to do with the particle when MaxCollisions is reached
 */
UENUM()
enum EParticleCollisionComplete
{
	/**	Kill the particle when MaxCollisions is reached		*/
	EPCC_Kill,
	/**	Freeze the particle in place						*/
	EPCC_Freeze,
	/**	Stop collision checks, but keep updating			*/
	EPCC_HaltCollisions,
	/**	Stop translations of the particle					*/
	EPCC_FreezeTranslation,
	/**	Stop rotations of the particle						*/
	EPCC_FreezeRotation,
	/**	Stop all movement of the particle					*/
	EPCC_FreezeMovement,
	EPCC_MAX,
};

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, abstract, meta=(DisplayName = "Collision"))
class UParticleModuleCollisionBase : public UParticleModule
{
	GENERATED_UCLASS_BODY()

};

