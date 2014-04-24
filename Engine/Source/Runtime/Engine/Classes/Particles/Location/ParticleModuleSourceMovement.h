// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ParticleModuleSourceMovement.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, meta=(DisplayName = "Source Movement"))
class UParticleModuleSourceMovement : public UParticleModuleLocationBase
{
	GENERATED_UCLASS_BODY()

	/** 
	 *	The scale factor to apply to the source movement before adding to the particle location.
	 *	The value is looked up using the particles RELATIVE time [0..1].
	 */
	UPROPERTY(EditAnywhere, Category=SourceMovement)
	struct FRawDistributionVector SourceMovementScale;

	//Begin UObject Interface
	virtual void PostInitProperties() OVERRIDE;
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	//End UObject Interface

	//Begin UParticleModule Interface
	virtual void FinalUpdate(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) OVERRIDE;
	virtual bool CanTickInAnyThread() OVERRIDE
	{
		return false;
	}
	//End UParticleModule Interface
};



