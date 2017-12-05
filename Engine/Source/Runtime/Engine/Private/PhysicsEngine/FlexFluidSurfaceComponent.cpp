// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
FlexFluidSurfaceComponent.cpp: Fluid surface implementation.
=============================================================================*/

#include "PhysicsEngine/FlexFluidSurfaceComponent.h"
#include "PhysicsEngine/FlexFluidSurface.h"
#include "PhysicsEngine/FlexFluidSurfaceActor.h"

#include "FlexFluidSurfaceSceneProxy.h"

/*=============================================================================
UFlexFluidSurfaceComponent
=============================================================================*/

UFlexFluidSurfaceComponent::UFlexFluidSurfaceComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, FlexFluidSurface(NULL)
	, MaxParticles(0)
{
	// tick driven through container
	PrimaryComponentTick.bCanEverTick = false;

	bAutoActivate = true;
	bReceivesDecals = false;
	bSelectable = false;
	bRenderInMainPass = true;
	bRenderCustomDepth = false;
	bUseAsOccluder = false;

	ParticleBounds.Init();
}

void UFlexFluidSurfaceComponent::SendRenderDynamicData_Concurrent()
{
	Super::SendRenderDynamicData_Concurrent();

	if (SceneProxy == nullptr || FlexFluidSurface == nullptr)
		return;

	check(MaxParticles > 0);

	if (Particles.Max() < int32(MaxParticles))
	{
		Particles.Reserve(MaxParticles);
		ParticleAnisotropies.Reserve(MaxParticles);
	}

	UMaterialInterface* RenderMaterial = FlexFluidSurface->Material;
	if (RenderMaterial == NULL || (RenderMaterial->CheckMaterialUsage_Concurrent(MATUSAGE_FlexFluidSurfaces) == false))
	{
		RenderMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
	}

	FFlexFluidSurfaceProperties* SurfaceProperties = new FFlexFluidSurfaceProperties;

	SurfaceProperties->Material = RenderMaterial;
	SurfaceProperties->CastParticleShadows = FlexFluidSurface->CastParticleShadows;
	SurfaceProperties->ReceivePrecachedShadows = FlexFluidSurface->ReceivePrecachedShadows;
	SurfaceProperties->SmoothingRadius = FlexFluidSurface->SmoothingRadius;
	SurfaceProperties->MaxRadialSamples = FlexFluidSurface->MaxRadialSamples;
	SurfaceProperties->DepthEdgeFalloff = FlexFluidSurface->DepthEdgeFalloff;
	SurfaceProperties->ThicknessParticleScale = FlexFluidSurface->ThicknessParticleScale;
	SurfaceProperties->DepthParticleScale = FlexFluidSurface->DepthParticleScale;
	SurfaceProperties->ShadowParticleScale = FlexFluidSurface->ShadowParticleScale;
	SurfaceProperties->TexResScale = FlexFluidSurface->HalfRes ? 0.5f: 1.0f;

	//create particle buffer and copy necessary data from component
	UFlexFluidSurfaceComponent::Particle* ParticleBufferTmp = nullptr;
	UFlexFluidSurfaceComponent::ParticleAnisotropy* ParticleAnisotropyBufferTmp = nullptr;
	if (Particles.Num() > 0)
	{
		ParticleBufferTmp = (UFlexFluidSurfaceComponent::Particle*)FMemory::Malloc(sizeof(UFlexFluidSurfaceComponent::Particle)*Particles.Num());
		FMemory::Memcpy(ParticleBufferTmp, &Particles[0], sizeof(UFlexFluidSurfaceComponent::Particle)*Particles.Num());
	}
	if (ParticleAnisotropies.Num() > 0)
	{
		ParticleAnisotropyBufferTmp = (UFlexFluidSurfaceComponent::ParticleAnisotropy*)FMemory::Malloc(sizeof(UFlexFluidSurfaceComponent::ParticleAnisotropy)*ParticleAnisotropies.Num());
		FMemory::Memcpy(ParticleAnisotropyBufferTmp, &ParticleAnisotropies[0], sizeof(UFlexFluidSurfaceComponent::ParticleAnisotropy)*ParticleAnisotropies.Num());
	}

	SurfaceProperties->ParticleBuffer = ParticleBufferTmp;
	SurfaceProperties->ParticleAnisotropyBuffer = ParticleAnisotropyBufferTmp;
	SurfaceProperties->NumParticles = Particles.Num();
	SurfaceProperties->MaxParticles = Particles.Max();

	// Shadow scene proxy
	FPrimitiveSceneProxy* ShadowSceneProxy = nullptr;
	if (GetAttachChildren().Num() > 0)
	{
		ShadowSceneProxy = ((UPrimitiveComponent*)GetAttachChildren()[0])->SceneProxy;
	}

	// Enqueue command to send to render thread
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		FSendFlexFluidSurfaceDynamicData,
		FFlexFluidSurfaceSceneProxy*, SceneProxy, (FFlexFluidSurfaceSceneProxy*)SceneProxy,
		FFlexFluidSurfaceShadowSceneProxy*, ShadowSceneProxy, (FFlexFluidSurfaceShadowSceneProxy*)ShadowSceneProxy,
		FFlexFluidSurfaceProperties*, SurfaceProperties, SurfaceProperties,
		{
			SceneProxy->SetDynamicData_RenderThread(SurfaceProperties);
			if (ShadowSceneProxy)
			{
				ShadowSceneProxy->SetDynamicData_RenderThread(SurfaceProperties);
			}
		});
}

void UFlexFluidSurfaceComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const 
{
	Super::GetUsedMaterials(OutMaterials, bGetDebugMaterials);

	if (FlexFluidSurface && FlexFluidSurface->Material)
		OutMaterials.Add(FlexFluidSurface->Material);
}

FBoxSphereBounds UFlexFluidSurfaceComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	if (ParticleBounds.IsValid)
	{
		return FBoxSphereBounds(ParticleBounds);
	}
	else
	{
		return FBoxSphereBounds(LocalToWorld.GetLocation(), FVector(0.0f, 0.0f, 0.0f), 0.0f);
	}
}

FPrimitiveSceneProxy* UFlexFluidSurfaceComponent::CreateSceneProxy()
{
	return new FFlexFluidSurfaceSceneProxy(this);
}

void UFlexFluidSurfaceComponent::NotifyParticleBatch(const FBox& ParticleBatchBounds)
{
	if (!ParticleBounds.IsValid)
	{
		ParticleBounds = ParticleBatchBounds;
	}
	else if (ParticleBounds.IsValid)
	{
		ParticleBounds += ParticleBatchBounds;
	}
	UpdateComponentToWorld();
	MarkRenderDynamicDataDirty();
}

void UFlexFluidSurfaceComponent::ClearParticles()
{
	Particles.SetNum(0, false);
	ParticleAnisotropies.SetNum(0, false);
	ParticleBounds.Init();
}

bool UFlexFluidSurfaceComponent::ShouldDisableEmitterRendering() const
{
	return FlexFluidSurface->DisableEmitterRendering;
}

/*=============================================================================
UFlexFluidSurfaceShadowComponent
=============================================================================*/

UFlexFluidSurfaceShadowComponent::UFlexFluidSurfaceShadowComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, MaxParticles(0)
{
	// tick driven through UFlexFluidSurfaceComponent
	PrimaryComponentTick.bCanEverTick = false;
	bAutoActivate = true;
	bReceivesDecals = false;
	bSelectable = false;
	bRenderInMainPass = false;
	bRenderCustomDepth = false;
	bUseAsOccluder = false;
}

void UFlexFluidSurfaceShadowComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	Super::GetUsedMaterials(OutMaterials, bGetDebugMaterials);

	UFlexFluidSurfaceComponent* SurfaceComponent = (UFlexFluidSurfaceComponent*)GetAttachParent();

	if (SurfaceComponent && SurfaceComponent->FlexFluidSurface && SurfaceComponent->FlexFluidSurface->Material)
		OutMaterials.Add(SurfaceComponent->FlexFluidSurface->Material);
}

FBoxSphereBounds UFlexFluidSurfaceShadowComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	USceneComponent* SurfaceComponent = GetAttachParent();
	return (SurfaceComponent != nullptr) ? SurfaceComponent->CalcBounds(LocalToWorld) : FBoxSphereBounds(LocalToWorld.GetLocation(), FVector(0.0f, 0.0f, 0.0f), 0.0f);
}

FPrimitiveSceneProxy* UFlexFluidSurfaceShadowComponent::CreateSceneProxy()
{
	return new FFlexFluidSurfaceShadowSceneProxy(this);
}

/*=============================================================================
UFlexFluidSurface
=============================================================================*/

UFlexFluidSurface::UFlexFluidSurface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SmoothingRadius = 10.0f;
	MaxRadialSamples = 5;
	DepthEdgeFalloff = 0.05f;
	ThicknessParticleScale = 2.0f;
	DepthParticleScale = 1.0f;
	ShadowParticleScale = 1.0f;
	HalfRes = false;
	CastParticleShadows = true;
	ReceivePrecachedShadows = true;
	DisableEmitterRendering = true;
	Material = NULL;
}

#if WITH_EDITOR
void UFlexFluidSurface::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	SmoothingRadius = FMath::Clamp(SmoothingRadius, 0.0f, 1000.0f);
	MaxRadialSamples = FMath::Clamp(MaxRadialSamples, 0, 100);

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif // WITH_EDITOR

/*=============================================================================
AFlexFluidSurfaceActor
=============================================================================*/

UFlexFluidSurfaceComponent* AFlexFluidSurfaceActor::SpawnActor(UFlexFluidSurface* FlexFluidSurface, uint32 MaxParticles, UWorld* World)
{
	check(FlexFluidSurface != nullptr && World != nullptr);
	FActorSpawnParameters ActorSpawnParameters;
	AFlexFluidSurfaceActor* NewActor = World->SpawnActor<AFlexFluidSurfaceActor>(AFlexFluidSurfaceActor::StaticClass(), ActorSpawnParameters);
	check(NewActor);
	UFlexFluidSurfaceComponent* FluidSurfaceComponent = (UFlexFluidSurfaceComponent*)NewActor->GetRootComponent();
	FluidSurfaceComponent->FlexFluidSurface = FlexFluidSurface;
	FluidSurfaceComponent->MaxParticles = MaxParticles;
	return FluidSurfaceComponent;
}

AFlexFluidSurfaceActor::AFlexFluidSurfaceActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UFlexFluidSurfaceShadowComponent* ShadowComponent = ObjectInitializer.CreateDefaultSubobject<UFlexFluidSurfaceShadowComponent>(this, TEXT("FlexFluidSurfaceShadowComponent"));
	RootComponent = ObjectInitializer.CreateDefaultSubobject<UFlexFluidSurfaceComponent>(this, TEXT("FlexFluidSurfaceComponent"));
	ShadowComponent->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);

	bHidden = false;
}
