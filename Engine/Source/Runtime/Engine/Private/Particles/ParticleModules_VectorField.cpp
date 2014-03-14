// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
	ParticleModules_VectorField.cpp: Vector field module implementations.
==============================================================================*/

#include "EnginePrivate.h"
#include "FXSystem.h"
#include "ParticleDefinitions.h"
#include "../DistributionHelpers.h"

/*------------------------------------------------------------------------------
	Global vector field scale.
------------------------------------------------------------------------------*/
UParticleModuleVectorFieldBase::UParticleModuleVectorFieldBase(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

UParticleModuleVectorFieldGlobal::UParticleModuleVectorFieldGlobal(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UParticleModuleVectorFieldGlobal::CompileModule(FParticleEmitterBuildInfo& EmitterInfo)
{
	EmitterInfo.GlobalVectorFieldScale = GlobalVectorFieldScale;
	EmitterInfo.GlobalVectorFieldTightness = GlobalVectorFieldTightness;
}

/*------------------------------------------------------------------------------
	Per-particle vector field scale.
------------------------------------------------------------------------------*/
UParticleModuleVectorFieldScale::UParticleModuleVectorFieldScale(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UParticleModuleVectorFieldScale::PostInitProperties()
{
	Super::PostInitProperties();
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad))
	{
		UDistributionFloatConstant* DistributionVectorFieldScale = NewNamedObject<UDistributionFloatConstant>(this, TEXT("DistributionVectorFieldScale"));
		DistributionVectorFieldScale->Constant = 1.0f;
		VectorFieldScale = DistributionVectorFieldScale;
	}
}

void UParticleModuleVectorFieldScale::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if (Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_MOVE_DISTRIBUITONS_TO_POSTINITPROPS)
	{
		FDistributionHelpers::RestoreDefaultConstant(VectorFieldScale, TEXT("DistributionVectorFieldScale"), 1.0f);
	}
}

void UParticleModuleVectorFieldScale::CompileModule(FParticleEmitterBuildInfo& EmitterInfo)
{
	EmitterInfo.VectorFieldScale.ScaleByDistribution(VectorFieldScale);
}

#if WITH_EDITOR

bool UParticleModuleVectorFieldScale::IsValidForLODLevel(UParticleLODLevel* LODLevel, FString& OutErrorString)
{
	if (LODLevel->TypeDataModule && LODLevel->TypeDataModule->IsA(UParticleModuleTypeDataGpu::StaticClass()))
	{
		if(!IsDistributionAllowedOnGPU(VectorFieldScale))
		{
			OutErrorString = GetDistributionNotAllowedOnGPUText(StaticClass()->GetName(), "VectorFieldScale" ).ToString();
			return false;
		}
	}

	return true;
}

#endif
/*------------------------------------------------------------------------------
	Per-particle vector field scale over life.
------------------------------------------------------------------------------*/
UParticleModuleVectorFieldScaleOverLife::UParticleModuleVectorFieldScaleOverLife(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UParticleModuleVectorFieldScaleOverLife::PostInitProperties()
{
	Super::PostInitProperties();
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad))
	{
		UDistributionFloatConstant* DistributionVectorFieldScaleOverLife = NewNamedObject<UDistributionFloatConstant>(this, TEXT("DistributionVectorFieldScaleOverLife"));
		DistributionVectorFieldScaleOverLife->Constant = 1.0f;
		VectorFieldScaleOverLife = DistributionVectorFieldScaleOverLife;
	}
}

void UParticleModuleVectorFieldScaleOverLife::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if (Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_MOVE_DISTRIBUITONS_TO_POSTINITPROPS)
	{
		FDistributionHelpers::RestoreDefaultConstant(VectorFieldScaleOverLife, TEXT("DistributionVectorFieldScaleOverLife"), 1.0f);
	}
}

void UParticleModuleVectorFieldScaleOverLife::CompileModule(FParticleEmitterBuildInfo& EmitterInfo)
{
	EmitterInfo.VectorFieldScaleOverLife.ScaleByDistribution(VectorFieldScaleOverLife);
}

#if WITH_EDITOR

bool UParticleModuleVectorFieldScaleOverLife::IsValidForLODLevel(UParticleLODLevel* LODLevel, FString& OutErrorString)
{
	if (LODLevel->TypeDataModule && LODLevel->TypeDataModule->IsA(UParticleModuleTypeDataGpu::StaticClass()))
	{
		if(!IsDistributionAllowedOnGPU(VectorFieldScaleOverLife))
		{
			OutErrorString = GetDistributionNotAllowedOnGPUText(StaticClass()->GetName(), "VectorFieldScaleOverLife" ).ToString();
			return false;
		}
	}

	return true;
}

#endif
/*------------------------------------------------------------------------------
	Local vector fields.
------------------------------------------------------------------------------*/
UParticleModuleVectorFieldLocal::UParticleModuleVectorFieldLocal(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	RelativeScale3D = FVector(1.0f, 1.0f, 1.0f);

	Intensity = 1.0;
	Tightness = 0.0;
}

void UParticleModuleVectorFieldLocal::CompileModule(FParticleEmitterBuildInfo& EmitterInfo)
{
	EmitterInfo.LocalVectorField = VectorField;
	EmitterInfo.LocalVectorFieldTransform.SetTranslation(RelativeTranslation);
	EmitterInfo.LocalVectorFieldTransform.SetRotation(RelativeRotation.Quaternion());
	EmitterInfo.LocalVectorFieldTransform.SetScale3D(RelativeScale3D);
	EmitterInfo.LocalVectorFieldIntensity = Intensity;
	EmitterInfo.LocalVectorFieldTightness = Tightness;
	EmitterInfo.bLocalVectorFieldIgnoreComponentTransform = bIgnoreComponentTransform;
	EmitterInfo.bLocalVectorFieldTileX = bTileX;
	EmitterInfo.bLocalVectorFieldTileY = bTileY;
	EmitterInfo.bLocalVectorFieldTileZ = bTileZ;
}

/*------------------------------------------------------------------------------
	Local vector initial rotation.
------------------------------------------------------------------------------*/
UParticleModuleVectorFieldRotation::UParticleModuleVectorFieldRotation(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UParticleModuleVectorFieldRotation::CompileModule(FParticleEmitterBuildInfo& EmitterInfo)
{
	EmitterInfo.LocalVectorFieldMinInitialRotation = MinInitialRotation;
	EmitterInfo.LocalVectorFieldMaxInitialRotation = MaxInitialRotation;
}

/*------------------------------------------------------------------------------
	Local vector field rotation rate.
------------------------------------------------------------------------------*/
UParticleModuleVectorFieldRotationRate::UParticleModuleVectorFieldRotationRate(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UParticleModuleVectorFieldRotationRate::CompileModule(FParticleEmitterBuildInfo& EmitterInfo)
{
	EmitterInfo.LocalVectorFieldRotationRate += RotationRate;
}
