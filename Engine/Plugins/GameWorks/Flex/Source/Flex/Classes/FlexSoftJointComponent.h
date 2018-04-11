// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "FlexContainer.h"
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/SceneComponent.h"
#include "CollisionQueryParams.h"
#include "FlexSoftJointComponent.generated.h"

struct NvFlexExtSoftJoint;
struct FFlexContainerInstance;

/**
*	Used to emit a soft joint that can affect flex objects in the same container.
*/
UCLASS(hidecategories = (Object, Mobility, LOD, Physics), ClassGroup = Physics, showcategories = Trigger, meta = (BlueprintSpawnableComponent))
class FLEX_API UFlexSoftJointComponent : public USceneComponent, public IFlexContainerClient
{
	GENERATED_UCLASS_BODY()

	/** The radius to apply the soft joint in */
	UPROPERTY(interp, EditAnywhere, BlueprintReadWrite, Category = SoftJointComponent)
	float Radius;

	/** Stiffness parameter allows small scale elastic deformation  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SoftJointComponent, meta = (ClampMin = 0.0, ClampMax = 1.0, UIMin = 0.0, UIMax = 1.0))
	float Stiffness;

	/** The simulation container the joint belongs to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SoftJointComponent, meta = (editcondition = "OverrideAsset"))
	UFlexContainer* ContainerTemplate;

	/* The simulation container the joint belongs to */
	FFlexContainerInstance* ContainerInstance;

	/** The global particle indice*/
	TArray<int32> ParticleIndices;

	/** The relative offsets from the particles to the center of mass of the joint */
	TArray<FVector> ParticleLocalPositions;

	/** How many particles affected by this soft joint */
	int32 NumParticles;

	/** Whether it has been initialized */
	bool bJointIsInitialized;

	NvFlexExtSoftJoint* SoftJoint;

public:

	/** Returns the number of particles in the joint**/
	FORCEINLINE int32 GetNumParticles() const { return NumParticles; }

	/**
	* Get the joint container template
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|SoftJoint")
	virtual UFlexContainer* GetContainerTemplate();

	// Begin IFlexContainerClient Interface
	virtual bool IsEnabled() { return true; }
	virtual FBoxSphereBounds GetBounds() { return Bounds; }
	virtual void Synchronize() {}
	// End IFlexContainerClient Interface

protected:

	virtual void OnUnregister() override;

protected:

	//~ Begin UActorComponent Interfac
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override; 
	//~ End UActorComponent Interface.
};



