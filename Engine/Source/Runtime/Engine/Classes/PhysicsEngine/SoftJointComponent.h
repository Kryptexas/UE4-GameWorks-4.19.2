// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Components/SceneComponent.h"
#include "CollisionQueryParams.h"
#include "SoftJointComponent.generated.h"

struct NvFlexExtJoint;

/**
*	Used to emit a soft joint that can affect flex objects.
*/
UCLASS(hidecategories = (Object, Mobility, LOD, Physics), ClassGroup = Physics, showcategories = Trigger, meta = (BlueprintSpawnableComponent))
class ENGINE_API USoftJointComponent : public USceneComponent, public IFlexContainerClient
{
	GENERATED_UCLASS_BODY()

	/** The radius to apply the soft joint in */
	UPROPERTY(interp, EditAnywhere, BlueprintReadWrite, Category = SoftJointComponent)
	float Radius;

	/** Stiffness parameter allows small scale elastic deformation  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SoftJointComponent)
	float Stiffness;

	/** The simulation container the joint belongs to */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = SoftJointComponent, meta = (editcondition = "OverrideAsset"))
	UFlexContainer* ContainerTemplate;

	/** The global particle indice*/
	TArray<int32> ParticleIndices;

	/** The relative offsets from the particles of the joint to the center */
	TArray<FVector> ParticleLocalPositions;

	/** How many particles affected by this soft joint */
	int32 NumParticles;

	/** Whether it has been initialized */
	bool JointIsInitialized;

	NvFlexExtJoint* Joint;

	/** Add an object type for this soft joint to affect */
	UFUNCTION(BlueprintCallable, Category = "Physics|Components|SoftJoint")
	virtual void AddObjectTypeToAffect(TEnumAsByte<enum EObjectTypeQuery> ObjectType);

	/** Remove an object type that is affected by this soft joint */
	UFUNCTION(BlueprintCallable, Category = "Physics|Components|SoftJoint")
	virtual void RemoveObjectTypeToAffect(TEnumAsByte<enum EObjectTypeQuery> ObjectType);

	/** Add a collision channel for this soft joint to affect */
	void AddCollisionChannelToAffect(enum ECollisionChannel CollisionChannel);

public:
	/* The simulation container the joint belongs to */
	FFlexContainerInstance* ContainerInstance;

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

	/** The object types that are affected by this radial force */
	UPROPERTY(EditAnywhere, Category = RadialForceComponent)
		TArray<TEnumAsByte<enum EObjectTypeQuery> > ObjectTypesToAffect;

	/** Cached object query params derived from ObjectTypesToAffect */
	FCollisionObjectQueryParams CollisionObjectQueryParams;

protected:

	//~ Begin UActorComponent Interfac
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override; 

	virtual void BeginPlay() override;
	//~ End UActorComponent Interface.

	//~ Begin UObject Interface.
	virtual void PostLoad() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject Interface.

	/** Update CollisionObjectQueryParams from ObjectTypesToAffect */
	void UpdateCollisionObjectQueryParams();
};



