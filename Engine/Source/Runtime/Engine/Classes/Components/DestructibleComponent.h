// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once



#include "DestructibleComponent.generated.h"

#if WITH_PHYSX
namespace physx
{
#if WITH_APEX
	namespace apex
	{
		class  NxDestructibleActor;
		struct NxApexDamageEventReportData;
	}
#endif
	class PxRigidDynamic;
}

/** Mapping info for destructible chunk user data. */
struct FDestructibleChunkInfo
{
	/** Index of this chunk info */
	int32 Index;
	/** Index of the chunk this data belongs to*/
	int32 ChunkIndex;
	/** Component owning this chunk info*/
	TWeakObjectPtr<class UDestructibleComponent> OwningComponent;
	/** Physx actor */
	physx::PxRigidDynamic* Actor;
};
#endif // WITH_PHYSX 

/**
 *	This component holds the physics data for a DestructibleActor
 *
 *	The USkeletalMesh pointer in the base class (SkinnedMeshComponent) MUST be a DestructibleMesh
 */
UCLASS(HeaderGroup=Component, ClassGroup=Physics, hidecategories=(Object,Mesh,"Components|SkinnedMesh",Mirroring,Activation,"Components|Activation"), config=Engine, editinlinenew, meta=(BlueprintSpawnableComponent))
class ENGINE_API UDestructibleComponent : public USkinnedMeshComponent
{
	GENERATED_UCLASS_BODY()

	/** If set, use this actor's fracture effects instead of the asset's fracture effects. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=DestructibleComponent)
	uint32 bFractureEffectOverride:1;

	/** Fracture effects for each fracture level. Used only if Fracture Effect Override is set. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, editfixedsize, Category=DestructibleComponent)
	TArray<struct FFractureEffect> FractureEffects;

	/**
	 *	Enable "hard sleeping" for destruction-generated PxActors.  This means that they turn kinematic
	 *	when they sleep, but can be made dynamic again by application of enough damage.
	 */	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=DestructibleComponent)
	bool bEnableHardSleeping;

#if WITH_EDITORONLY_DATA
	/** Provide a blueprint interface for setting the destructible mesh */
	UPROPERTY(Transient, EditAnywhere, BlueprintReadWrite, Category=DestructibleComponent)
	class UDestructibleMesh* DestructibleMesh;
#endif // WITH_EDITORONLY_DATA

#if WITH_PHYSX
	/** Per chunk info */
	TArray<FDestructibleChunkInfo> ChunkInfos;
#endif // WITH_PHYSX 

#if WITH_EDITOR
	// Begin UObject interface.
	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	// End UObject interface.

	// Take damage
	UFUNCTION(BlueprintCallable, Category="Components|Destructible")
	void ApplyDamage(float DamageAmount, const FVector& HitLocation, const FVector& ImpulseDir, float ImpulseStrength);

	// Take radius damage
	UFUNCTION(BlueprintCallable, Category="Components|Destructible")
	void ApplyRadiusDamage(float BaseDamage, const FVector& HurtOrigin, float DamageRadius, float ImpulseStrength, bool bFullDamage);

	UFUNCTION(BlueprintCallable, Category="Components|Destructible")
	void SetDestructibleMesh(class UDestructibleMesh* NewMesh);

	UFUNCTION(BlueprintCallable, Category="Components|Destructible")
	class UDestructibleMesh * GetDestructibleMesh();
public:
#if WITH_APEX
	/** The NxDestructibleActor instantated from an NxDestructibleAsset, which contains the runtime physical state. */
	physx::apex::NxDestructibleActor* ApexDestructibleActor;
#endif	//WITH_APEX

	// Begin USceneComponent interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform & LocalToWorld) const OVERRIDE;
	virtual void OnUpdateTransform(bool bSkipPhysicsMove) OVERRIDE;
	virtual void Activate(bool bReset=false) OVERRIDE;
	virtual void Deactivate() OVERRIDE;
	// End USceneComponent interface.

	// Begin UActorComponent interface.
	virtual void CreatePhysicsState() OVERRIDE;
	virtual void DestroyPhysicsState() OVERRIDE;
	virtual class UBodySetup* GetBodySetup() OVERRIDE;
	// End UActorComponent interface.

	// Begin UPrimitiveComponent interface.
	virtual FBodyInstance* GetBodyInstance(FName BoneName = NAME_None) const OVERRIDE;
	virtual bool IsAnySimulatingPhysics() const OVERRIDE;

	virtual void AddImpulse(FVector Impulse, FName BoneName = NAME_None, bool bVelChange = false) OVERRIDE;
	virtual void AddImpulseAtLocation(FVector Impulse, FVector Position, FName BoneName = NAME_None) OVERRIDE;
	virtual void AddForce(FVector Force, FName BoneName = NAME_None) OVERRIDE;
	virtual void AddForceAtLocation(FVector Force, FVector Location, FName BoneName = NAME_None) OVERRIDE;
	virtual void AddRadialImpulse(FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff, bool bVelChange=false) OVERRIDE;
	virtual void AddRadialForce(FVector Origin, float Radius, float Strength, ERadialImpulseFalloff Falloff) OVERRIDE;
	virtual void ReceiveComponentDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) OVERRIDE;
	virtual void PostPhysicsTick(FPrimitiveComponentPostPhysicsTickFunction &ThisTickFunction) OVERRIDE;

	virtual bool LineTraceComponent( FHitResult& OutHit, const FVector Start, const FVector End, const FCollisionQueryParams& Params ) OVERRIDE;
	virtual bool SweepComponent( FHitResult& OutHit, const FVector Start, const FVector End, const FCollisionShape& CollisionShape, bool bTraceComplex=false) OVERRIDE;
	virtual bool ShouldTrackOverlaps() const { return true; };
	// End UPrimitiveComponent interface.

	// Begin SkinnedMeshComponent interface.
	virtual bool ShouldUpdateTransform(bool bLODHasChanged) const OVERRIDE;
	virtual void RefreshBoneTransforms() OVERRIDE;
	virtual void SetSkeletalMesh(USkeletalMesh* InSkelMesh) OVERRIDE;
	virtual FTransform GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace = RTS_World) const OVERRIDE;
	// End SkinnedMeshComponent interface.


	// Begin DestructibleComponent interface.
	
	struct FFakeBodyInstanceState
	{
		physx::PxRigidActor* ActorSync;
		physx::PxRigidActor* ActorAsync;
		int32 InstanceIndex;
	};

#if WITH_APEX
	/** Changes the body instance to have the specified actor and instance id. */
	void SetupFakeBodyInstance(physx::PxRigidActor* NewRigidActor, int32 InstanceIdx, FFakeBodyInstanceState* PrevState = NULL);
	
	/** Resets the BodyInstance to the state that is defined in PrevState. */
	void ResetFakeBodyInstance(FFakeBodyInstanceState& PrevState);
#endif // WITH_APEX

	/** This method makes a chunk (fractured piece) visible or invisible.
	 *
	 * @param ChunkIndex - Which chunk to affect.  ChunkIndex must lie in the range: 0 <= ChunkIndex < ((DestructibleMesh*)USkeletalMesh)->ApexDestructibleAsset->chunkCount().
	 * @param bVisible - If true, the chunk will be made visible.  Otherwise, the chunk is made invisible.
	 */
	void SetChunkVisible( int32 ChunkIndex, bool bVisible );

	/** This method sets a chunk's (fractured piece's) world rotation and translation.
	 *
	 * @param ChunkIndex - Which chunk to affect.  ChunkIndex must lie in the range: 0 <= ChunkIndex < ((DestructibleMesh*)USkeletalMesh)->ApexDestructibleAsset->chunkCount().
	 * @param WorldRotation - The orienation to give to the chunk in world space, represented as a quaternion.
	 * @param WorldRotation - The world space position to give to the chunk.
	 */
	void SetChunkWorldRT( int32 ChunkIndex, const FQuat& WorldRotation, const FVector& WorldTranslation );

#if WITH_APEX
	/** Trigger any fracture effects after a damage event is recieved */
	void SpawnFractureEffectsFromDamageEvent(const physx::apex::NxApexDamageEventReportData& InDamageEvent);

	/** Callback from physics system to notify the actor that it has been damaged */
	void OnDamageEvent(const physx::apex::NxApexDamageEventReportData& InDamageEvent);
#endif // WITH_APEX

	// End DestructibleComponent interface.

	virtual bool DoCustomNavigableGeometryExport(struct FNavigableGeometryExport* GeomExport) const OVERRIDE;

	FORCEINLINE static int32 ChunkIdxToBoneIdx(int32 ChunkIdx) { return ChunkIdx + 1; }
	FORCEINLINE static int32 BoneIdxToChunkIdx(int32 BoneIdx) { return FMath::Max(BoneIdx - 1, 0); }
private:
	/** Collision response used for chunks */
	FCollisionResponse ChunkCollisionResponse;
#if WITH_PHYSX
	/** User data wrapper for this component passed to physx */
	FPhysxUserData PhysxUserData;

	/** User data wrapper for the chunks passed to physx */
	TArray<FPhysxUserData> PhysxChunkUserData;

	void SetCollisionResponseForActor(const FCollisionResponse& ColResponse, physx::PxRigidDynamic* Actor, int32 ChunkIdx);
#endif
};



