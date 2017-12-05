// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "Components/PrimitiveComponent.h"

#include "FlexFluidSurfaceComponent.generated.h"

/**
 *	Used to render a screen space fluid surface for particles.
 */
UCLASS(ClassGroup = Rendering, collapsecategories, hidecategories = (Object, Mobility, Collision, Physics, PhysicsVolume, Actor))
class ENGINE_API UFlexFluidSurfaceComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

public:

	void NotifyParticleBatch(const FBox& ParticleBounds);
	void ClearParticles();

	bool ShouldDisableEmitterRendering() const;

	class UFlexFluidSurface* FlexFluidSurface;
	uint32 MaxParticles;

	struct Particle
	{
		FVector Position;
		float Size;
		FLinearColor Color;
	};

	struct ParticleAnisotropy
	{
		FVector4 Anisotropy1;
		FVector4 Anisotropy2;
		FVector4 Anisotropy3;
	};

	TArray<Particle> Particles;
	TArray<ParticleAnisotropy> ParticleAnisotropies;

	// Begin UActorComponent interface.
protected:
	virtual void SendRenderDynamicData_Concurrent() override;
	// End UActorComponent interface.

	// Begin USceneComponent interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const override;
	// Begin USceneComponent interface.

	// Begin UPrimitiveComponent interface.
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	// End UPrimitiveComponent interface.


private:
	FBox ParticleBounds;
};

/**
*	Used to render particle based shadows for screen space fluid surface.
*/
UCLASS(ClassGroup = Rendering, collapsecategories)
class ENGINE_API UFlexFluidSurfaceShadowComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

public:
	uint32 MaxParticles;

protected:
	// Begin USceneComponent interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const override;
	// Begin USceneComponent interface.

	// Begin UPrimitiveComponent interface.
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	// End UPrimitiveComponent interface.
};
