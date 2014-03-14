// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ParticleModules_Material.cpp: 
	Material-related particle module implementations.
=============================================================================*/
#include "EnginePrivate.h"
#include "ParticleDefinitions.h"

UParticleModuleMaterialBase::UParticleModuleMaterialBase(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

/*-----------------------------------------------------------------------------
	Abstract base modules used for categorization.
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	UParticleModuleMeshMaterial
-----------------------------------------------------------------------------*/
UParticleModuleMeshMaterial::UParticleModuleMeshMaterial(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bSpawnModule = true;
	bUpdateModule = true;
}

	//## BEGIN PROPS ParticleModuleMeshMaterial
//	TArray<class UMaterialInstance*> MeshMaterials;
	//## END PROPS ParticleModuleMeshMaterial

void UParticleModuleMeshMaterial::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{

}

uint32 UParticleModuleMeshMaterial::RequiredBytesPerInstance(FParticleEmitterInstance* Owner)
{
	// Cheat and setup the emitter instance material array here...
	if (Owner && bEnabled)
	{
		Owner->SetMeshMaterials( MeshMaterials );
	}
	return 0;
}
