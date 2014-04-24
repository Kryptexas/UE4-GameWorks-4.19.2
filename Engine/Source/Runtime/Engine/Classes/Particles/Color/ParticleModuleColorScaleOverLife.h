// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 *	ParticleModuleColorScaleOverLife
 *
 *	The base class for all Beam modules.
 *
 */

#pragma once
#include "ParticleModuleColorScaleOverLife.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, MinimalAPI, meta=(DisplayName = "Scale Color / Life"))
class UParticleModuleColorScaleOverLife : public UParticleModuleColorBase
{
	GENERATED_UCLASS_BODY()

	/** The scale factor for the color.													*/
	UPROPERTY(EditAnywhere, Category=Color)
	struct FRawDistributionVector ColorScaleOverLife;

	/** The scale factor for the alpha.													*/
	UPROPERTY(EditAnywhere, Category=Color)
	struct FRawDistributionFloat AlphaScaleOverLife;

	/** Whether it is EmitterTime or ParticleTime related.								*/
	UPROPERTY(EditAnywhere, Category=Color)
	uint32 bEmitterTime:1;

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
	virtual void SetToSensibleDefaults(UParticleEmitter* Owner) OVERRIDE;
#if WITH_EDITOR
	virtual int32 GetNumberOfCustomMenuOptions() const OVERRIDE;
	virtual bool GetCustomMenuEntryDisplayString(int32 InEntryIndex, FString& OutDisplayString) const OVERRIDE;
	virtual bool PerformCustomMenuEntry(int32 InEntryIndex) OVERRIDE;
#endif
	//End UParticleModule Interface
};



