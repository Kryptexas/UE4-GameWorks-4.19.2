// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 *	ParticleModuleLocationDirect
 *
 *	Sets the location of particles directly.
 *
 */

#pragma once
#include "ParticleModuleLocationDirect.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, MinimalAPI, meta=(DisplayName = "Direct Location"))
class UParticleModuleLocationDirect : public UParticleModuleLocationBase
{
	GENERATED_UCLASS_BODY()

	/** 
	 *	The location of the particle at a give time. Retrieved using the particle RelativeTime. 
	 *	IMPORTANT: the particle location is set to this value, thereby over-writing any previous module impacts.
	 */
	UPROPERTY(EditAnywhere, Category=Location)
	struct FRawDistributionVector Location;

	/**
	 *	An offset to apply to the position retrieved from the Location calculation. 
	 *	The offset is retrieved using the EmitterTime. 
	 *	The offset will remain constant over the life of the particle.
	 */
	UPROPERTY(EditAnywhere, Category=Location)
	struct FRawDistributionVector LocationOffset;

	/**
	 *	Scales the velocity of the object at a given point in the time-line.
	 */
	UPROPERTY(EditAnywhere, Category=Location)
	struct FRawDistributionVector ScaleFactor;

	/** Currently unused. */
	UPROPERTY(EditAnywhere, Category=Location)
	struct FRawDistributionVector Direction;

	/** Initializes the default values for this property */
	void InitializeDefaults();

	// Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	virtual void	PostInitProperties() OVERRIDE;
	virtual void	Serialize(FArchive& Ar) OVERRIDE;
	// End UObject Interface

	// Begin UParticleModule Interface
	virtual void	Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) OVERRIDE;
	virtual void	Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) OVERRIDE;
	virtual uint32	RequiredBytes(FParticleEmitterInstance* Owner = NULL) OVERRIDE;
	// End UParticleModule Interface
};



