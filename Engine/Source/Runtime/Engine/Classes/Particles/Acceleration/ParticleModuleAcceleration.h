// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ParticleModuleAcceleration.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, meta=(DisplayName = "Acceleration"), MinimalAPI)
class UParticleModuleAcceleration : public UParticleModuleAccelerationBase
{
	GENERATED_UCLASS_BODY()

	/**
	 *	The initial acceleration of the particle.
	 *	Value is obtained using the EmitterTime at particle spawn.
	 *	Each frame, the current and base velocity of the particle 
	 *	is then updated using the formula 
	 *		velocity += acceleration * DeltaTime
	 *	where DeltaTime is the time passed since the last frame.
	 */
	UPROPERTY(EditAnywhere, Category=Acceleration)
	struct FRawDistributionVector Acceleration;

	/**
	 *	If true, then apply the particle system components scale 
	 *	to the acceleration value.
	 */
	UPROPERTY(EditAnywhere, Category=Acceleration)
	uint32 bApplyOwnerScale:1;

	/** Initializes the default values for this property */
	void InitializeDefaults();

	//Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	virtual void PostInitProperties() OVERRIDE;
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	//End UObject Interface

	//Begin UParticleModule Interface
	virtual void CompileModule( struct FParticleEmitterBuildInfo& EmitterInfo ) OVERRIDE;
	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) OVERRIDE;
	virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) OVERRIDE;
	virtual uint32 RequiredBytes(FParticleEmitterInstance* Owner = NULL) OVERRIDE;

#if WITH_EDITOR
	virtual bool IsValidForLODLevel(UParticleLODLevel* LODLevel, FString& OutErrorString) OVERRIDE;
#endif
	//End UParticleModule Interface

protected:
	friend class FParticleModuleAccelerationDetails;
};



