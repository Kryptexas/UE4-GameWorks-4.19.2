// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ParticleModuleMeshRotation.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, MinimalAPI, meta=(DisplayName = "Init Mesh Rotation"))
class UParticleModuleMeshRotation : public UParticleModuleRotationBase
{
	GENERATED_UCLASS_BODY()

	/**
	 *	Initial rotation in ROTATIONS PER SECOND (1 = 360 degrees).
	 *	The value is retrieved using the EmitterTime.
	 */
	UPROPERTY(EditAnywhere, Category=Rotation)
	struct FRawDistributionVector StartRotation;

	/** If true, apply the parents rotation as well. */
	UPROPERTY(EditAnywhere, Category=Rotation)
	uint32 bInheritParent:1;

	//Begin UObject Interface
	virtual void	PostInitProperties() OVERRIDE;
	virtual void	Serialize(FArchive& Ar) OVERRIDE;
	//End UObject Interface

	//Begin UParticleModule Interface
	virtual void	Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) OVERRIDE;
	virtual bool	TouchesMeshRotation() const OVERRIDE { return true; }
	//End UParticleModule Interface

	/**
	 *	Extended version of spawn, allows for using a random stream for distribution value retrieval
	 *
	 *	@param	Owner				The particle emitter instance that is spawning
	 *	@param	Offset				The offset to the modules payload data
	 *	@param	SpawnTime			The time of the spawn
	 *	@param	InRandomStream		The random stream to use for retrieving random values
	 */
	void SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, class FRandomStream* InRandomStream, FBaseParticle* ParticleBase);

};



