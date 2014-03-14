// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PhysicsAsset.generated.h"

#define		RB_MinSizeToLockDOF				(0.1)
#define		RB_MinAngleToLockDOF			(5.0)

/**
 * Endian save storage for a pair of rigid body indices used as a key in the CollisionDisableTable TMap.
 */
struct FRigidBodyIndexPair
{
	/** Pair of indices */
	int32		Indices[2];
	
	/** Default constructor required for use with TMap */
	FRigidBodyIndexPair()
	{}

	/**
	 * Constructor, taking unordered pair of indices and generating a key.
	 *
	 * @param	Index1	1st unordered index
	 * @param	Index2	2nd unordered index
	 */
	FRigidBodyIndexPair( int32 Index1, int32 Index2 )
	{
		Indices[0] = FMath::Min( Index1, Index2 );
		Indices[1] = FMath::Max( Index1, Index2 );
	}

	/**
	 * == operator required for use with TMap
	 *
	 * @param	Other	FRigidBodyIndexPair to compare this one to.
	 * @return	true if the passed in FRigidBodyIndexPair is identical to this one, false otherwise
	 */
	bool operator==( const FRigidBodyIndexPair &Other ) const
	{
		return (Indices[0] == Other.Indices[0]) && (Indices[1] == Other.Indices[1]);
	}

	/**
	 * Serializes the rigid body index pair to the passed in archive.	
	 *
	 * @param	Ar		Archive to serialize data to.
	 * @param	Pair	FRigidBodyIndexPair to serialize
	 * @return	returns the passed in FArchive after using it for serialization
	 */
	friend FArchive& operator<< ( FArchive &Ar, FRigidBodyIndexPair &Pair )
	{
		Ar << Pair.Indices[0] << Pair.Indices[1];
		return Ar;
	}
};

/**
 * Generates a hash value in accordance with the uint64 implementation of GetTypeHash which is
 * required for backward compatibility as older versions of UPhysicsAssetInstance stored
 * a TMap<uint64,bool>.
 * 
 * @param	Pair	FRigidBodyIndexPair to calculate the hash value for
 * @return	the hash value for the passed in FRigidBodyIndexPair
 */
inline uint32 GetTypeHash( const FRigidBodyIndexPair Pair )
{
	return Pair.Indices[0] + (Pair.Indices[1] * 23);
}


UCLASS(hidecategories=Object, BlueprintType, MinimalAPI)
class UPhysicsAsset : public UObject
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITORONLY_DATA
	/** 
	 *	Default skeletal mesh to use when previewing this PhysicsAsset etc. 
	 *	Is the one that was used as the basis for creating this Asset.
	 */
	UPROPERTY()
	class USkeletalMesh * DefaultSkelMesh_DEPRECATED;

	UPROPERTY()
	TAssetPtr<class USkeletalMesh> PreviewSkeletalMesh;

#endif // WITH_EDITORONLY_DATA
	/** 
	 *	Array of BodySetup objects. Stores information about collision shape etc. for each body.
	 *	Does not include body position - those are taken from mesh.
	 */
	UPROPERTY(instanced)
	TArray<class UBodySetup*> BodySetup;

	/** Index of bodies that are marked bConsiderForBounds */
	UPROPERTY()
	TArray<int32> BoundsBodies;

	/** 
	 *	Array of RB_ConstraintSetup objects. 
	 *	Stores information about a joint between two bodies, such as position relative to each body, joint limits etc.
	 */
	UPROPERTY(instanced)
	TArray<class UPhysicsConstraintTemplate*> ConstraintSetup;

public:
	/** This caches the BodySetup Index by BodyName to speed up FindBodyIndex */
	TMap<FName, int32>					BodySetupIndexMap;

	/** 
	 *	Table indicating which pairs of bodies have collision disabled between them. Used internally. 
	 *	Note, this is accessed from within physics engine, so is not safe to change while physics is running
	 */
	TMap<FRigidBodyIndexPair,bool>		CollisionDisableTable;

	// Begin UObject interface
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	virtual void PostLoad() OVERRIDE;
	virtual FString GetDesc() OVERRIDE;
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const OVERRIDE;
#if WITH_EDITOR
	virtual void PostEditUndo() OVERRIDE;
#endif
	// End UObject interface

	// Find the index of the physics bone that is controlling this graphics bone.
	ENGINE_API int32		FindControllingBodyIndex(class USkeletalMesh* skelMesh, int32 BoneIndex);
	ENGINE_API int32		FindConstraintIndex(FName ConstraintName);
	FName					FindConstraintBoneName(int32 ConstraintIndex);

	/** Utility for getting indices of all bodies below (and including) the one with the supplied name. */
	ENGINE_API void			GetBodyIndicesBelow(TArray<int32>& OutBodyIndices, FName InBoneName, USkeletalMesh* InSkelMesh);

	ENGINE_API FBox			CalcAABB(const class USkinnedMeshComponent* MeshComponent) const;

	/** Clears physics meshes from all bodies */
	ENGINE_API void ClearAllPhysicsMeshes();
	
#if WITH_EDITOR
	/** Invalidates physics meshes from all bodies. Data will be rebuilt completely. */
	ENGINE_API void InvalidateAllPhysicsMeshes();
#endif

	// @todo document
	void DrawCollision(class FPrimitiveDrawInterface* PDI, const USkeletalMesh* SkelMesh, const TArray<FTransform>& SpaceBases, const FTransform& LocalToWorld, float Scale);

	// @todo document
	void DrawConstraints(class FPrimitiveDrawInterface* PDI, const USkeletalMesh* SkelMesh, const TArray<FTransform>& SpaceBases, const FTransform& LocalToWorld, float Scale);


	// @todo document
	ENGINE_API void DisableCollision(int32 BodyIndexA, int32 BodyIndexB);

	// @todo document
	ENGINE_API void EnableCollision(int32 BodyIndexA, int32 BodyIndexB);

	/** Update the BoundsBodies array and cache the indices of bodies marked with bConsiderForBounds to BoundsBodies array. */
	ENGINE_API void UpdateBoundsBodiesArray();

	/** Update the BodySetup Array Index Map.  */
	ENGINE_API void UpdateBodySetupIndexMap();


	// @todo document
	ENGINE_API int32 FindBodyIndex(FName BodyName);
};

