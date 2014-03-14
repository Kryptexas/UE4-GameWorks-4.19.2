// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 *
 */

#pragma once
#include "EmitterSpawnable.generated.h"

UCLASS(abstract)
class AEmitterSpawnable : public AEmitter
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(replicatedUsing=OnRep_ParticleTemplate)
	class UParticleSystem* ParticleTemplate;

	/** Replication Notification Callbacks */
	UFUNCTION()
	virtual void OnRep_ParticleTemplate();


	// Begin AEmitter Interface
	virtual void SetTemplate(class UParticleSystem* NewTemplate) OVERRIDE;
	// End AEmitter Interface
};



