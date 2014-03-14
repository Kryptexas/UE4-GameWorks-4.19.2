// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
	ParticleModuleAccelerationDrag: Drag coefficient.
==============================================================================*/

#pragma once

#include "ParticleModuleAccelerationDrag.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=(Object, Acceleration), MinimalAPI, meta=(DisplayName = "Drag"))
class UParticleModuleAccelerationDrag : public UParticleModuleAccelerationBase
{
	GENERATED_UCLASS_BODY()

	/** Per-particle drag coefficient. Evaluted using emitter time. */
	UPROPERTY(EditAnywhere, Category=Drag)
	class UDistributionFloat* DragCoefficient;

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
	virtual void Update( FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime ) OVERRIDE;
	//End UParticleModule Interface

#if WITH_EDITOR
	virtual bool IsValidForLODLevel(UParticleLODLevel* LODLevel, FString& OutErrorString) OVERRIDE;
#endif

protected:
	friend class FParticleModuleAccelerationDragDetails;
};



