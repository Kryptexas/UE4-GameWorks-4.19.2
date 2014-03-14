// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "ParticleModulePivotOffset.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, MinimalAPI, DisplayName="Pivot Offset")
class UParticleModulePivotOffset : public UParticleModuleLocationBase
{
	GENERATED_UCLASS_BODY()

	/** Offset applied in UV space to the particle vertex positions. Defaults to (0.5,0.5) putting the pivot in the centre of the partilce. */
	UPROPERTY(EditAnywhere, Category=PivotOffset)
	FVector2D PivotOffset;

	/** Initializes the default values for this property */
	void InitializeDefaults();

	//Begin UObject Interface
	virtual void PostInitProperties() OVERRIDE;
	//End UObject Interface

	//Begin UParticleModule Interface
	virtual void CompileModule( struct FParticleEmitterBuildInfo& EmitterInfo ) OVERRIDE;
	//End UParticleModule Interface

#if WITH_EDITOR
	virtual bool IsValidForLODLevel(UParticleLODLevel* LODLevel, FString& OutErrorString) OVERRIDE;
#endif

};



