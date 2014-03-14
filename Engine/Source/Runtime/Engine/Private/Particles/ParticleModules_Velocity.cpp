// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	ParticleModules_Velocity.cpp: 
	Velocity-related particle module implementations.
=============================================================================*/

#include "EnginePrivate.h"
#include "ParticleDefinitions.h"
#include "../DistributionHelpers.h"

UParticleModuleVelocityBase::UParticleModuleVelocityBase(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

/*-----------------------------------------------------------------------------
	Abstract base modules used for categorization.
-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
	UParticleModuleVelocity implementation.
-----------------------------------------------------------------------------*/

UParticleModuleVelocity::UParticleModuleVelocity(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bSpawnModule = true;
}

void UParticleModuleVelocity::InitializeDefaults()
{
	if (!StartVelocity.Distribution)
	{
		StartVelocity.Distribution = NewNamedObject<UDistributionVectorUniform>(this, TEXT("DistributionStartVelocity"));
	}

	if (!StartVelocityRadial.Distribution)
	{
		StartVelocityRadial.Distribution = NewNamedObject<UDistributionFloatUniform>(this, TEXT("DistributionStartVelocityRadial"));
	}
}

void UParticleModuleVelocity::PostInitProperties()
{
	Super::PostInitProperties();
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad))
	{
		InitializeDefaults();
	}
}

void UParticleModuleVelocity::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if (Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_MOVE_DISTRIBUITONS_TO_POSTINITPROPS)
	{
		FDistributionHelpers::RestoreDefaultUniform(StartVelocity.Distribution, TEXT("DistributionStartVelocity"), FVector::ZeroVector, FVector::ZeroVector);
		FDistributionHelpers::RestoreDefaultUniform(StartVelocityRadial.Distribution, TEXT("DistributionStartVelocityRadial"), 0.0f, 0.0f);
	}
}

#if WITH_EDITOR
void UParticleModuleVelocity::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	InitializeDefaults();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif // WITH_EDITOR

void UParticleModuleVelocity::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	SpawnEx(Owner, Offset, SpawnTime, NULL, ParticleBase);
}

void UParticleModuleVelocity::SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, class FRandomStream* InRandomStream, FBaseParticle* ParticleBase)
{
	SPAWN_INIT;
	{
		FVector Vel = StartVelocity.GetValue(Owner->EmitterTime, Owner->Component, 0, InRandomStream);
		FVector FromOrigin = (Particle.Location - Owner->EmitterToSimulation.GetOrigin()).SafeNormal();

		FVector OwnerScale(1.0f);
		if ((bApplyOwnerScale == true) && Owner && Owner->Component)
		{
			OwnerScale = Owner->Component->ComponentToWorld.GetScale3D();
		}

		UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
		check(LODLevel);
		if (LODLevel->RequiredModule->bUseLocalSpace)
		{
			if (bInWorldSpace == true)
			{
				Vel = Owner->SimulationToWorld.InverseTransformVector(Vel);
			}
			else
			{
				Vel = Owner->EmitterToSimulation.TransformVector(Vel);
			}
		}
		else if (bInWorldSpace == false)
		{
			Vel = Owner->EmitterToSimulation.TransformVector(Vel);
		}
		Vel *= OwnerScale;
		Vel += FromOrigin * StartVelocityRadial.GetValue(Owner->EmitterTime, Owner->Component, InRandomStream) * OwnerScale;
		Particle.Velocity		+= Vel;
		Particle.BaseVelocity	+= Vel;
	}
}

/*-----------------------------------------------------------------------------
	UParticleModuleVelocityInheritParent implementation.
-----------------------------------------------------------------------------*/
UParticleModuleVelocity_Seeded::UParticleModuleVelocity_Seeded(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bSpawnModule = true;
	bSupportsRandomSeed = true;
	bRequiresLoopingNotification = true;
}

void UParticleModuleVelocity_Seeded::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	FParticleRandomSeedInstancePayload* Payload = (FParticleRandomSeedInstancePayload*)(Owner->GetModuleInstanceData(this));
	SpawnEx(Owner, Offset, SpawnTime, (Payload != NULL) ? &(Payload->RandomStream) : NULL, ParticleBase);
}

uint32 UParticleModuleVelocity_Seeded::RequiredBytesPerInstance(FParticleEmitterInstance* Owner)
{
	return RandomSeedInfo.GetInstancePayloadSize();
}

uint32 UParticleModuleVelocity_Seeded::PrepPerInstanceBlock(FParticleEmitterInstance* Owner, void* InstData)
{
	return PrepRandomSeedInstancePayload(Owner, (FParticleRandomSeedInstancePayload*)InstData, RandomSeedInfo);
}

void UParticleModuleVelocity_Seeded::EmitterLoopingNotify(FParticleEmitterInstance* Owner)
{
	if (RandomSeedInfo.bResetSeedOnEmitterLooping == true)
	{
		FParticleRandomSeedInstancePayload* Payload = (FParticleRandomSeedInstancePayload*)(Owner->GetModuleInstanceData(this));
		PrepRandomSeedInstancePayload(Owner, Payload, RandomSeedInfo);
	}
}

/*-----------------------------------------------------------------------------
	UParticleModuleVelocityInheritParent implementation.
-----------------------------------------------------------------------------*/
UParticleModuleVelocityInheritParent::UParticleModuleVelocityInheritParent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bSpawnModule = true;
}

void UParticleModuleVelocityInheritParent::InitializeDefaults()
{
	if (!Scale.Distribution)
	{
		UDistributionVectorConstant* DistributionScale = NewNamedObject<UDistributionVectorConstant>(this, TEXT("DistributionScale"));
		DistributionScale->Constant = FVector(1.0f, 1.0f, 1.0f);
		Scale.Distribution = DistributionScale;
	}
}

void UParticleModuleVelocityInheritParent::PostInitProperties()
{
	Super::PostInitProperties();
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad))
	{
		InitializeDefaults();
	}
}

void UParticleModuleVelocityInheritParent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if (Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_MOVE_DISTRIBUITONS_TO_POSTINITPROPS)
	{
		FDistributionHelpers::RestoreDefaultConstant(Scale.Distribution, TEXT("DistributionScale"), FVector(1.0f, 1.0f, 1.0f));
	}
}

#if WITH_EDITOR
void UParticleModuleVelocityInheritParent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	InitializeDefaults();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif // WITH_EDITOR

void UParticleModuleVelocityInheritParent::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	SPAWN_INIT;

	FVector Vel(0.0f);

	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	if (LODLevel->RequiredModule->bUseLocalSpace)
	{
		Vel = Owner->Component->ComponentToWorld.InverseTransformVector(Owner->Component->PartSysVelocity);
	}
	else
	{
		Vel = Owner->Component->PartSysVelocity;
	}

	FVector vScale = Scale.GetValue(Owner->EmitterTime, Owner->Component);

	Vel *= vScale;

	Particle.Velocity		+= Vel;
	Particle.BaseVelocity	+= Vel;
}

/*-----------------------------------------------------------------------------
	UParticleModuleVelocityOverLifetime implementation.
-----------------------------------------------------------------------------*/
UParticleModuleVelocityOverLifetime::UParticleModuleVelocityOverLifetime(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bSpawnModule = true;
	bUpdateModule = true;

	Absolute = false;
}

void UParticleModuleVelocityOverLifetime::InitializeDefaults()
{
	if (!VelOverLife.Distribution)
	{
		VelOverLife.Distribution = NewNamedObject<UDistributionVectorConstantCurve>(this, TEXT("DistributionVelOverLife"));
	}
}

void UParticleModuleVelocityOverLifetime::PostInitProperties()
{
	Super::PostInitProperties();
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad))
	{
		InitializeDefaults();
	}
}

#if WITH_EDITOR
void UParticleModuleVelocityOverLifetime::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	InitializeDefaults();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif // WITH_EDITOR

void UParticleModuleVelocityOverLifetime::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	if (Absolute)
	{
		SPAWN_INIT;
		FVector OwnerScale(1.0f);
		if ((bApplyOwnerScale == true) && Owner && Owner->Component)
		{
			OwnerScale = Owner->Component->ComponentToWorld.GetScale3D();
		}
		FVector Vel = VelOverLife.GetValue(Particle.RelativeTime, Owner->Component) * OwnerScale;
		Particle.Velocity		= Vel;
		Particle.BaseVelocity	= Vel;
	}
}

void UParticleModuleVelocityOverLifetime::Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime)
{
	check(Owner && Owner->Component);
	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	FVector OwnerScale(1.0f);
	if ((bApplyOwnerScale == true) && Owner && Owner->Component)
	{
		OwnerScale = Owner->Component->ComponentToWorld.GetScale3D();
	}
	if (Absolute)
	{
		if (LODLevel->RequiredModule->bUseLocalSpace == false)
		{
			if (bInWorldSpace == false)
			{
				FVector Vel;
				const FMatrix LocalToWorld = Owner->Component->ComponentToWorld.ToMatrixNoScale();
				BEGIN_UPDATE_LOOP;
				{
					Vel = VelOverLife.GetValue(Particle.RelativeTime, Owner->Component);
					Particle.Velocity = LocalToWorld.TransformVector(Vel) * OwnerScale;
				}
				END_UPDATE_LOOP;
			}
			else
			{
				BEGIN_UPDATE_LOOP;
				{
					Particle.Velocity = VelOverLife.GetValue(Particle.RelativeTime, Owner->Component) * OwnerScale;
				}
				END_UPDATE_LOOP;
			}
		}
		else
		{
			if (bInWorldSpace == false)
			{
				BEGIN_UPDATE_LOOP;
				{
					Particle.Velocity = VelOverLife.GetValue(Particle.RelativeTime, Owner->Component) * OwnerScale;
				}
				END_UPDATE_LOOP;
			}
			else
			{
				FVector Vel;
				const FMatrix LocalToWorld = Owner->Component->ComponentToWorld.ToMatrixNoScale();
				const FMatrix InvMat = LocalToWorld.Inverse();
				BEGIN_UPDATE_LOOP;
				{
					Vel = VelOverLife.GetValue(Particle.RelativeTime, Owner->Component);
					Particle.Velocity = InvMat.TransformVector(Vel) * OwnerScale;
				}
				END_UPDATE_LOOP;
			}
		}
	}
	else
	{
		if (LODLevel->RequiredModule->bUseLocalSpace == false)
		{
			FVector Vel;
			if (bInWorldSpace == false)
			{
				const FMatrix LocalToWorld = Owner->Component->ComponentToWorld.ToMatrixNoScale();
				BEGIN_UPDATE_LOOP;
				{
					Vel = VelOverLife.GetValue(Particle.RelativeTime, Owner->Component);
					Particle.Velocity *= LocalToWorld.TransformVector(Vel) * OwnerScale;
				}
				END_UPDATE_LOOP;
			}
			else
			{
				BEGIN_UPDATE_LOOP;
				{
					Particle.Velocity *= VelOverLife.GetValue(Particle.RelativeTime, Owner->Component) * OwnerScale;
				}
				END_UPDATE_LOOP;
			}
		}
		else
		{
			if (bInWorldSpace == false)
			{
				BEGIN_UPDATE_LOOP;
				{
					Particle.Velocity *= VelOverLife.GetValue(Particle.RelativeTime, Owner->Component) * OwnerScale;
				}
				END_UPDATE_LOOP;
			}
			else
			{
				FVector Vel;
				const FMatrix LocalToWorld = Owner->Component->ComponentToWorld.ToMatrixNoScale();
				const FMatrix InvMat = LocalToWorld.Inverse();
				BEGIN_UPDATE_LOOP;
				{
					Vel = VelOverLife.GetValue(Particle.RelativeTime, Owner->Component);
					Particle.Velocity *= InvMat.TransformVector(Vel) * OwnerScale;
				}
				END_UPDATE_LOOP;
			}
		}
	}
}

/*-----------------------------------------------------------------------------
	UParticleModuleVelocityCone implementation.
-----------------------------------------------------------------------------*/
UParticleModuleVelocityCone::UParticleModuleVelocityCone(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bSpawnModule = true;
	bSupported3DDrawMode = true;
	Direction = FVector(0.0f, 0.0f, 1.0f);
}

void UParticleModuleVelocityCone::InitializeDefaults()
{
	if (!Angle.Distribution)
	{
		Angle.Distribution = NewNamedObject<UDistributionFloatUniform>(this, TEXT("DistributionAngle"));
	}

	if (!Velocity.Distribution)
	{
		Velocity.Distribution = NewNamedObject<UDistributionFloatUniform>(this, TEXT("DistributionVelocity"));
	}
}

void UParticleModuleVelocityCone::PostInitProperties()
{
	Super::PostInitProperties();
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad))
	{
		InitializeDefaults();
	}
}

void UParticleModuleVelocityCone::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	if (Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_MOVE_DISTRIBUITONS_TO_POSTINITPROPS)
	{
		FDistributionHelpers::RestoreDefaultUniform(Angle.Distribution, TEXT("DistributionAngle"), 0.0f, 0.0f);
		FDistributionHelpers::RestoreDefaultUniform(Velocity.Distribution, TEXT("DistributionVelocity"), 0.0f, 0.0f);
	}
}

#if WITH_EDITOR
void UParticleModuleVelocityCone::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	InitializeDefaults();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif // WITH_EDITOR

void UParticleModuleVelocityCone::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	SpawnEx(Owner, Offset, SpawnTime, NULL, ParticleBase);
}

void UParticleModuleVelocityCone::SpawnEx(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, class FRandomStream* InRandomStream, FBaseParticle* ParticleBase)
{
	static const float TwoPI = 2.0f * PI;
	static const float ToRads = PI / 180.0f;
	static const int32 UUPerRad = 10430;
	static const FVector DefaultDirection(0.0f, 0.0f, 1.0f);
	
	// Calculate the owning actor's scale
	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	FVector OwnerScale(1.0f);
	if ((bApplyOwnerScale == true) && Owner && Owner->Component)
	{
		OwnerScale = Owner->Component->ComponentToWorld.GetScale3D();
	}
	
	// Spawn particles
	SPAWN_INIT
	{
		// Calculate the default position (prior to the Direction vector being factored in)
		const float SpawnAngle = Angle.GetValue(Owner->EmitterTime, Owner->Component, InRandomStream);
		const float SpawnVelocity = Velocity.GetValue(Owner->EmitterTime, Owner->Component, InRandomStream);
		const float LatheAngle = FMath::SRand() * TwoPI;
		const FRotator DefaultDirectionRotater((int32)(SpawnAngle * ToRads * UUPerRad), (int32)(LatheAngle * UUPerRad), 0);
		const FRotationMatrix DefaultDirectionRotation(DefaultDirectionRotater);
		const FVector DefaultSpawnDirection = DefaultDirectionRotation.TransformVector(DefaultDirection);

		// Orientate the cone along the direction vector		
		const FVector ForwardDirection = (Direction != FVector::ZeroVector)? Direction.SafeNormal(): DefaultDirection;
		FVector UpDirection(0.0f, 0.0f, 1.0f);
		FVector RightDirection(1.0f, 0.0f, 0.0f);

		if ((ForwardDirection != UpDirection) && (-ForwardDirection != UpDirection))
		{
			RightDirection = UpDirection ^ ForwardDirection;
			UpDirection = ForwardDirection ^ RightDirection;
		}
		else
		{
			UpDirection = ForwardDirection ^ RightDirection;
			RightDirection = UpDirection ^ ForwardDirection;
		}

		FMatrix DirectionRotation;
		DirectionRotation.SetIdentity();
		DirectionRotation.SetAxis(0, RightDirection.SafeNormal());
		DirectionRotation.SetAxis(1, UpDirection.SafeNormal());
		DirectionRotation.SetAxis(2, ForwardDirection);
		FVector SpawnDirection = DirectionRotation.TransformVector(DefaultSpawnDirection);
	
		// Transform according to world and local space flags 
		if (!LODLevel->RequiredModule->bUseLocalSpace && !bInWorldSpace)
		{
			SpawnDirection = Owner->Component->ComponentToWorld.TransformVector(SpawnDirection);
		}
		else if (LODLevel->RequiredModule->bUseLocalSpace && bInWorldSpace)
		{
			SpawnDirection = Owner->Component->ComponentToWorld.InverseTransformVector(SpawnDirection);
		}

		// Set final velocity vector
		const FVector FinalVelocity = SpawnDirection * SpawnVelocity * OwnerScale;
		Particle.Velocity += FinalVelocity;
		Particle.BaseVelocity += FinalVelocity;
	}
}

void UParticleModuleVelocityCone::Render3DPreview(FParticleEmitterInstance* Owner, const FSceneView* View,FPrimitiveDrawInterface* PDI)
{
#if WITH_EDITOR
	float ConeMaxAngle = 0.0f;
	float ConeMinAngle = 0.0f;
	Angle.GetOutRange(ConeMinAngle, ConeMaxAngle);

	float ConeMaxVelocity = 0.0f;
	float ConeMinVelocity = 0.0f;
	Velocity.GetOutRange(ConeMinVelocity, ConeMaxVelocity);

	float MaxLifetime = 0.0f;
	TArray<UParticleModule*>& Modules = Owner->SpriteTemplate->GetCurrentLODLevel(Owner)->Modules;
	for (int32 ModuleIndex = 0; ModuleIndex < Modules.Num(); ModuleIndex++)
	{
		UParticleModuleLifetimeBase* LifetimeMod = Cast<UParticleModuleLifetimeBase>(Modules[ModuleIndex]);
		if (LifetimeMod != NULL)
		{
			MaxLifetime = LifetimeMod->GetMaxLifetime();
			break;
		}
	}

	const int32 ConeSides = 16;
	const float ConeRadius = ConeMaxVelocity * MaxLifetime;

	// Calculate direction transform
	const FVector DefaultDirection(0.0f, 0.0f, 1.0f);
	const FVector ForwardDirection = (Direction != FVector::ZeroVector)? Direction.SafeNormal(): DefaultDirection;
	FVector UpDirection(0.0f, 0.0f, 1.0f);
	FVector RightDirection(1.0f, 0.0f, 0.0f);

	if ((ForwardDirection != UpDirection) && (-ForwardDirection != UpDirection))
	{
		RightDirection = UpDirection ^ ForwardDirection;
		UpDirection = ForwardDirection ^ RightDirection;
	}
	else
	{
		UpDirection = ForwardDirection ^ RightDirection;
		RightDirection = UpDirection ^ ForwardDirection;
	}

	FMatrix DirectionRotation;
	DirectionRotation.SetIdentity();
	DirectionRotation.SetAxis(0, RightDirection.SafeNormal());
	DirectionRotation.SetAxis(1, UpDirection.SafeNormal());
	DirectionRotation.SetAxis(2, ForwardDirection);

	// Calculate the owning actor's scale and rotation
	UParticleLODLevel* LODLevel	= Owner->SpriteTemplate->GetCurrentLODLevel(Owner);
	check(LODLevel);
	FVector OwnerScale(1.0f);
	FRotationMatrix OwnerRotation(FRotator::ZeroRotator);
	FVector LocalToWorldOrigin(0.0f);
	FMatrix LocalToWorld(FMatrix::Identity);
	if (Owner && Owner->Component)
	{
		AActor* Actor = Owner->Component->GetOwner();
		if (Actor)
		{
			if (bApplyOwnerScale == true)
			{
				OwnerScale = Owner->Component->ComponentToWorld.GetScale3D();
			}

			OwnerRotation = FRotationMatrix(Actor->GetActorRotation());
		}
	  LocalToWorldOrigin = Owner->Component->ComponentToWorld.GetLocation();
	  LocalToWorld = Owner->Component->ComponentToWorld.ToMatrixWithScale().RemoveTranslation();
	  LocalToWorld.RemoveScaling();
	}
	FMatrix Transform;
	Transform.SetIdentity();

	// DrawWireCone() draws a cone down the X axis, but this cone's default direction is down Z
	const FRotationMatrix XToZRotation(FRotator((int32)(HALF_PI * 10430), 0, 0));
	Transform *= XToZRotation;

	// Apply scale
	Transform.SetAxis(0, Transform.GetScaledAxis( EAxis::X ) * OwnerScale.X);
	Transform.SetAxis(1, Transform.GetScaledAxis( EAxis::Y ) * OwnerScale.Y);
	Transform.SetAxis(2, Transform.GetScaledAxis( EAxis::Z ) * OwnerScale.Z);

	// Apply direction transform
	Transform *= DirectionRotation;

	// Transform according to world and local space flags 
	if (!LODLevel->RequiredModule->bUseLocalSpace && !bInWorldSpace)
	{
		Transform *= LocalToWorld;
	}
	else if (LODLevel->RequiredModule->bUseLocalSpace && bInWorldSpace)
	{
		Transform *= OwnerRotation;
		Transform *= LocalToWorld.Inverse();
	}
	else if (!bInWorldSpace)
	{
		Transform *= OwnerRotation;
	}

	// Apply translation
	Transform.SetOrigin(LocalToWorldOrigin);

	TArray<FVector> OuterVerts;
	TArray<FVector> InnerVerts;

	// Draw inner and outer cones
	DrawWireCone(PDI, Transform, ConeRadius, ConeMinAngle, ConeSides, ModuleEditorColor, SDPG_World, InnerVerts);
	DrawWireCone(PDI, Transform, ConeRadius, ConeMaxAngle, ConeSides, ModuleEditorColor, SDPG_World, OuterVerts);

	// Draw radial spokes
	for (int32 i = 0; i < ConeSides; ++i)
	{
		PDI->DrawLine( OuterVerts[i], InnerVerts[i], ModuleEditorColor, SDPG_World );
	}
#endif
}
