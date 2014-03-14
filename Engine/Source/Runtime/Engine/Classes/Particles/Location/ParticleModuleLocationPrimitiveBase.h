// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
// ParticleModuleLocationPrimitiveBase
// Base class for setting particle spawn locations based on primitives.
//=============================================================================

#pragma once
#include "ParticleModuleLocationPrimitiveBase.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object)
class UParticleModuleLocationPrimitiveBase : public UParticleModuleLocationBase
{
	GENERATED_UCLASS_BODY()

	/** Whether the positive X axis is valid for spawning. */
	UPROPERTY(EditAnywhere, Category=Location)
	uint32 Positive_X:1;

	/** Whether the positive Y axis is valid for spawning. */
	UPROPERTY(EditAnywhere, Category=Location)
	uint32 Positive_Y:1;

	/** Whether the positive Z axis is valid for spawning. */
	UPROPERTY(EditAnywhere, Category=Location)
	uint32 Positive_Z:1;

	/** Whether the negative X axis is valid for spawning. */
	UPROPERTY(EditAnywhere, Category=Location)
	uint32 Negative_X:1;

	/** Whether the negative Y axis is valid for spawning. */
	UPROPERTY(EditAnywhere, Category=Location)
	uint32 Negative_Y:1;

	/** Whether the negative Zaxis is valid for spawning. */
	UPROPERTY(EditAnywhere, Category=Location)
	uint32 Negative_Z:1;

	/** Whether particles will only spawn on the surface of the primitive. */
	UPROPERTY(EditAnywhere, Category=Location)
	uint32 SurfaceOnly:1;

	/** Whether the particle should get its velocity from the position within the primitive. */
	UPROPERTY(EditAnywhere, Category=Location)
	uint32 Velocity:1;

	/** The scale applied to the velocity. (Only used if 'Velocity' is checked). */
	UPROPERTY(EditAnywhere, Category=Location)
	struct FRawDistributionFloat VelocityScale;

	/** The location of the bounding primitive relative to the position of the emitter. */
	UPROPERTY(EditAnywhere, Category=Location)
	struct FRawDistributionVector StartLocation;

	/** Initializes the default values for this property */
	void InitializeDefaults();

	//Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	virtual void PostInitProperties() OVERRIDE;
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	//End UObject Interface

	//@todo document
	virtual void	DetermineUnitDirection(FParticleEmitterInstance* Owner, FVector& vUnitDir, class FRandomStream* InRandomStream);
};



