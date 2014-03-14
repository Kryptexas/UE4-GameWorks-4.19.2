// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ParticleModules_Camera.cpp: 
	Camera-related particle module implementations.
=============================================================================*/
#include "EnginePrivate.h"
#include "ParticleDefinitions.h"
#include "../DistributionHelpers.h"

UParticleModuleCameraBase::UParticleModuleCameraBase(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

/*-----------------------------------------------------------------------------
	Abstract base modules used for categorization.
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	UParticleModuleCameraOffset
-----------------------------------------------------------------------------*/
UParticleModuleCameraOffset::UParticleModuleCameraOffset(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bSpawnModule = true;
	bUpdateModule = true;
	bSpawnTimeOnly = false;
	UpdateMethod = EPCOUM_DirectSet;
}

void UParticleModuleCameraOffset::InitializeDefaults()
{
	if (!CameraOffset.Distribution)
	{
		UDistributionFloatConstant* DistributionCameraOffset = NewNamedObject<UDistributionFloatConstant>(this, TEXT("DistributionCameraOffset")); 
		DistributionCameraOffset->Constant = 1.0f;
		CameraOffset.Distribution = DistributionCameraOffset; 
	}
}

void UParticleModuleCameraOffset::PostInitProperties()
{
	Super::PostInitProperties();
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad))
	{
		InitializeDefaults();
	}
}

void UParticleModuleCameraOffset::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if (Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_MOVE_DISTRIBUITONS_TO_POSTINITPROPS)
	{
		FDistributionHelpers::RestoreDefaultConstant(CameraOffset.Distribution, TEXT("DistributionCameraOffset"), 1.0f);
	}
}

#if WITH_EDITOR
void UParticleModuleCameraOffset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	InitializeDefaults();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif // WITH_EDITOR

void UParticleModuleCameraOffset::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	check(Owner && Owner->Component)
	float ScaleFactor = 1.0f;

	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	if (LODLevel->RequiredModule->bUseLocalSpace == false)
	{
		if (Owner != NULL)
		{
			if (Owner->Component != NULL)
			{
				ScaleFactor = Owner->Component->ComponentToWorld.GetMaximumAxisScale();
			}
		}
	}
	SPAWN_INIT;
	{
		CurrentOffset = Owner ? ((Owner->CameraPayloadOffset != 0) ? Owner->CameraPayloadOffset : Offset) : Offset;
		PARTICLE_ELEMENT(FCameraOffsetParticlePayload, CameraPayload);
		float CameraOffsetValue = CameraOffset.GetValue(Particle.RelativeTime, Owner->Component) * ScaleFactor;
		if (UpdateMethod == EPCOUM_DirectSet)
		{
			CameraPayload.BaseOffset = CameraOffsetValue;
			CameraPayload.Offset = CameraOffsetValue;
		}
		else if (UpdateMethod == EPCOUM_Additive)
		{
			CameraPayload.Offset += CameraOffsetValue;
		}
		else //if (UpdateMethod == EPCOUM_Scalar)
		{
			CameraPayload.Offset *= CameraOffsetValue;
		}
	}
}

void UParticleModuleCameraOffset::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
	if (bSpawnTimeOnly == false)
	{
		BEGIN_UPDATE_LOOP;
		{
			CurrentOffset = Owner ? ((Owner->CameraPayloadOffset != 0) ? Owner->CameraPayloadOffset : Offset) : Offset;
			PARTICLE_ELEMENT(FCameraOffsetParticlePayload, CameraPayload);
			float CameraOffsetValue = CameraOffset.GetValue(Particle.RelativeTime, Owner->Component);
			if (UpdateMethod == EPCOUM_Additive)
			{
				CameraPayload.Offset += CameraOffsetValue;
			}
			else if (UpdateMethod == EPCOUM_Scalar)
			{
				CameraPayload.Offset *= CameraOffsetValue;
			}
			else //if (UpdateMethod == EPCOUM_DirectSet)
			{
				CameraPayload.Offset = CameraOffsetValue;
			}
		}
		END_UPDATE_LOOP;
	}
}

uint32 UParticleModuleCameraOffset::RequiredBytes(FParticleEmitterInstance* Owner)
{
	if ((Owner == NULL) || (Owner->CameraPayloadOffset == 0))
	{
		return sizeof(FCameraOffsetParticlePayload);
	}
	// Only the first module needs to setup the payload
	return 0;
}
