// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ParticleModuleSpawn.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, hidecategories=ParticleModuleSpawnBase, MinimalAPI, meta=(DisplayName = "Spawn"))
class UParticleModuleSpawn : public UParticleModuleSpawnBase
{
	GENERATED_UCLASS_BODY()

	/** The rate at which to spawn particles. */
	UPROPERTY(EditAnywhere, Category=Spawn)
	struct FRawDistributionFloat Rate;

	/** The scalar to apply to the rate. */
	UPROPERTY(EditAnywhere, Category=Spawn)
	struct FRawDistributionFloat RateScale;

	/** The method to utilize when burst-emitting particles. */
	UPROPERTY(EditAnywhere, Category=Burst)
	TEnumAsByte<enum EParticleBurstMethod> ParticleBurstMethod;

	/** The array of burst entries. */
	UPROPERTY(EditAnywhere, export, noclear, Category=Burst)
	TArray<struct FParticleBurst> BurstList;

	/** Scale all burst entries by this amount. */
	UPROPERTY(EditAnywhere, Category=Burst)
	struct FRawDistributionFloat BurstScale;

	/** Initializes the default values for this property */
	void InitializeDefaults();

	// Begin UObject Interface
#if WITH_EDITOR
	virtual void	PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	virtual void	PostInitProperties() OVERRIDE;
	virtual void	Serialize(FArchive& Ar) OVERRIDE;
	// End UObject Interface

	// Begin UParticleModule Interface
	virtual bool	GenerateLODModuleValues(UParticleModule* SourceModule, float Percentage, UParticleLODLevel* LODLevel) OVERRIDE;
	// End UParticleModule Interface

	// Begin UParticleModuleSpawnBase Interface
	virtual bool GetSpawnAmount(FParticleEmitterInstance* Owner, int32 Offset, float OldLeftover, 
		float DeltaTime, int32& Number, float& Rate) OVERRIDE;
	virtual float GetMaximumSpawnRate() OVERRIDE;
	virtual float GetEstimatedSpawnRate() OVERRIDE;
	virtual int32 GetMaximumBurstCount() OVERRIDE;
	// End UParticleModuleSpawnBase Interface
};



