// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ParticleModuleMeshMaterial.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, MinimalAPI, meta=(DisplayName = "Mesh Material"))
class UParticleModuleMeshMaterial : public UParticleModuleMaterialBase
{
	GENERATED_UCLASS_BODY()

	/** The array of materials to apply to the mesh particles. */
	UPROPERTY(EditAnywhere, Category=MeshMaterials)
	TArray<class UMaterialInterface*> MeshMaterials;


	//Begin UParticleModule Interface
	virtual void	Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) OVERRIDE;
	virtual uint32	RequiredBytesPerInstance(FParticleEmitterInstance* Owner = NULL) OVERRIDE;
	//End UParticleModule Interface
};



