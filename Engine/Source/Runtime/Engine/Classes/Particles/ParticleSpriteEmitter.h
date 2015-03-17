// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "Particles/ParticleEmitter.h"
#include "ParticleSpriteEmitter.generated.h"

UENUM()
enum EParticleScreenAlignment
{
	PSA_FacingCameraPosition,
	PSA_Square,
	PSA_Rectangle,
	PSA_Velocity,
	PSA_AwayFromCenter,
	PSA_TypeSpecific,
	PSA_MAX,
};

UCLASS(collapsecategories, hidecategories=Object, editinlinenew, MinimalAPI)
class UParticleSpriteEmitter : public UParticleEmitter
{
	GENERATED_BODY()
public:
	ENGINE_API UParticleSpriteEmitter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin UObject Interface
	virtual void PostLoad() override;
	// End UObject Interface

	// Begin UParticleEmitter Interface
	virtual FParticleEmitterInstance* CreateInstance(UParticleSystemComponent* InComponent) override;
	virtual void SetToSensibleDefaults() override;
	// End UParticleEmitter Interface
};

