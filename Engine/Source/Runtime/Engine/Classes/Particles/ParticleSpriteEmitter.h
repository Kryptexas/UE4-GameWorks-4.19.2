// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ParticleSpriteEmitter.generated.h"

UENUM()
enum EParticleScreenAlignment
{
	PSA_FacingCameraPosition,
	PSA_Square,
	PSA_Rectangle,
	PSA_Velocity,
	PSA_TypeSpecific,
	PSA_MAX,
};

UCLASS(HeaderGroup=Particle, collapsecategories, hidecategories=Object, editinlinenew, MinimalAPI)
class UParticleSpriteEmitter : public UParticleEmitter
{
	GENERATED_UCLASS_BODY()


	// Begin UObject Interface
	virtual void PostLoad() OVERRIDE;
	// End UObject Interface

	// Begin UParticleEmitter Interface
	virtual FParticleEmitterInstance* CreateInstance(UParticleSystemComponent* InComponent) OVERRIDE;
	virtual void SetToSensibleDefaults() OVERRIDE;
	// End UParticleEmitter Interface
};

