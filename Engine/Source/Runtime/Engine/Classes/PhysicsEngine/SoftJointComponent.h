// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/SceneComponent.h"
#include "CollisionQueryParams.h"
#include "SoftJointComponent.generated.h"

/**
*	Used to emit a soft joint that can affect flex objects.
*/
UCLASS(hidecategories = (Object, Mobility, LOD, Physics), ClassGroup = Physics, showcategories = Trigger, meta = (BlueprintSpawnableComponent))
class ENGINE_API USoftJointComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

	/** The radius to apply the soft joint in */
	UPROPERTY(interp, EditAnywhere, BlueprintReadWrite, Category = SoftJointComponent)
		float Radius;

	/** Stiffness parameter allows small scale elastic deformation  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SoftJointComponent)
		float Stiffness;

private:
	/** The global particle indice*/
	TArray<int32> ParticleIndices;

	/** The relative offsets from the particle of the joint to the center */
	TArray<FVector> ParticleLocalPositions;

	/** How many particles affected by this soft joint */
	int32 NumParticles;

public:
	/** Returns the number of particles in the joint**/
	FORCEINLINE int32 GetNumParticles() const { return NumParticles; }

protected:

	virtual void BeginPlay() override;
	//~ End UActorComponent Interface.

	//~ Begin UObject Interface.
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface.
};



