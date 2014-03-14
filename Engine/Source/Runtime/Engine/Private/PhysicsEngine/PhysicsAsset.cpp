// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	PhysicsAsset.cpp
=============================================================================*/ 

#include "EnginePrivate.h"


#if WITH_PHYSX
	#include "PhysXSupport.h"
#endif // WITH_PHYSX


///////////////////////////////////////	
//////////// UPhysicsAsset ////////////
///////////////////////////////////////

UPhysicsAsset::UPhysicsAsset(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UPhysicsAsset::UpdateBoundsBodiesArray()
{
	BoundsBodies.Empty();

	for(int32 i=0; i<BodySetup.Num(); i++)
	{
		check(BodySetup[i]);
		if(BodySetup[i]->bConsiderForBounds)
		{
			BoundsBodies.Add(i);
		}
	}
}

void UPhysicsAsset::UpdateBodySetupIndexMap()
{
	// update BodySetupIndexMap
	BodySetupIndexMap.Empty();

	for(int32 i=0; i<BodySetup.Num(); i++)
	{
		check(BodySetup[i]);
		BodySetupIndexMap.Add(BodySetup[i]->BoneName, i);
	}
}

void UPhysicsAsset::PostLoad()
{
	Super::PostLoad();

	// Ensure array of bounds bodies is up to date.
	if(BoundsBodies.Num() == 0)
	{
		UpdateBoundsBodiesArray();
	}

	if (BodySetup.Num() > 0 && BodySetupIndexMap.Num() == 0)
	{
		UpdateBodySetupIndexMap();
	}
}

void UPhysicsAsset::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
	Ar << CollisionDisableTable;

#if WITH_EDITORONLY_DATA
	if (DefaultSkelMesh_DEPRECATED != NULL)
	{
		PreviewSkeletalMesh = TAssetPtr<USkeletalMesh>(DefaultSkelMesh_DEPRECATED);
		DefaultSkelMesh_DEPRECATED = NULL;
	}
#endif
}


void UPhysicsAsset::EnableCollision(int32 BodyIndexA, int32 BodyIndexB)
{
	if(BodyIndexA == BodyIndexB)
	{
		return;
	}

	FRigidBodyIndexPair Key(BodyIndexA, BodyIndexB);

	// If its not in table - do nothing
	if( !CollisionDisableTable.Find(Key) )
	{
		return;
	}

	CollisionDisableTable.Remove(Key);
}

void UPhysicsAsset::DisableCollision(int32 BodyIndexA, int32 BodyIndexB)
{
	if(BodyIndexA == BodyIndexB)
	{
		return;
	}

	FRigidBodyIndexPair Key(BodyIndexA, BodyIndexB);

	// If its already in the disable table - do nothing
	if( CollisionDisableTable.Find(Key) )
	{
		return;
	}

	CollisionDisableTable.Add(Key, 0);
}

FBox UPhysicsAsset::CalcAABB(const USkinnedMeshComponent* MeshComp) const
{
	FBox Box(0);

	if (!MeshComp)
	{
		return Box;
	}

	FVector Scale3D = MeshComp->ComponentToWorld.GetScale3D();
	if( Scale3D.IsUniform() )
	{
		const TArray<int32>* BodyIndexRefs = NULL;
		TArray<int32> AllBodies;
		// If we want to consider all bodies, make array with all body indices in
		if(MeshComp->bConsiderAllBodiesForBounds)
		{
			AllBodies.AddUninitialized(BodySetup.Num());
			for(int32 i=0; i<BodySetup.Num();i ++)
			{
				AllBodies[i] = i;
			}
			BodyIndexRefs = &AllBodies;
		}
		// Otherwise, use the cached shortlist of bodies to consider
		else
		{
			BodyIndexRefs = &BoundsBodies;
		}

		// Then iterate over bodies we want to consider, calculating bounding box for each
		const int32 BodySetupNum = (*BodyIndexRefs).Num();

		for(int32 i=0; i<BodySetupNum; i++)
		{
			const int32 BodyIndex = (*BodyIndexRefs)[i];
			UBodySetup* bs = BodySetup[BodyIndex];

			if (bs->bConsiderForBounds)
			{
				if (i+1<BodySetupNum)
				{
					int32 NextIndex = (*BodyIndexRefs)[i+1];
					FPlatformMisc::Prefetch(BodySetup[NextIndex]);
					FPlatformMisc::Prefetch(BodySetup[NextIndex], CACHE_LINE_SIZE);
				}

				int32 BoneIndex = MeshComp->GetBoneIndex(bs->BoneName);
				if(BoneIndex != INDEX_NONE)
				{
					FTransform WorldBoneTransform = MeshComp->GetBoneTransform(BoneIndex);
					if(FMath::Abs(WorldBoneTransform.GetDeterminant()) > (float)KINDA_SMALL_NUMBER)
					{
						Box += bs->AggGeom.CalcAABB( WorldBoneTransform );
					}
				}
			}
		}
	}
	else
	{
		UE_LOG(LogPhysics, Log,  TEXT("UPhysicsAsset::CalcAABB : Non-uniform scale factor. You will not be able to collide with it.  Turn off collision and wrap it with a blocking volume.  MeshComp: %s  SkelMesh: %s"), *MeshComp->GetFullName(), MeshComp->SkeletalMesh ? *MeshComp->SkeletalMesh->GetFullName() : TEXT("NULL") );
	}

	if(!Box.IsValid)
	{
		Box = FBox( MeshComp->GetComponentLocation(), MeshComp->GetComponentLocation() );
	}

	return Box;
}

int32	UPhysicsAsset::FindControllingBodyIndex(class USkeletalMesh* skelMesh, int32 StartBoneIndex)
{
	int32 BoneIndex = StartBoneIndex;
	while(BoneIndex!=INDEX_NONE)
	{
		FName BoneName = skelMesh->RefSkeleton.GetBoneName(BoneIndex);
		int32 BodyIndex = FindBodyIndex(BoneName);

		if(BodyIndex != INDEX_NONE)
			return BodyIndex;

		int32 ParentBoneIndex = skelMesh->RefSkeleton.GetParentIndex(BoneIndex);

		if(ParentBoneIndex == BoneIndex)
			return INDEX_NONE;

		BoneIndex = ParentBoneIndex;
	}

	return INDEX_NONE; // Shouldn't reach here.
}


int32 UPhysicsAsset::FindBodyIndex(FName bodyName)
{
	int32 * IdxData = BodySetupIndexMap.Find(bodyName);
	if (IdxData)
	{
		return *IdxData;
	}

	return INDEX_NONE;
}

int32 UPhysicsAsset::FindConstraintIndex(FName ConstraintName)
{
	for(int32 i=0; i<ConstraintSetup.Num(); i++)
	{
		if( ConstraintSetup[i]->DefaultInstance.JointName == ConstraintName )
		{
			return i;
		}
	}

	return INDEX_NONE;
}

FName UPhysicsAsset::FindConstraintBoneName(int32 ConstraintIndex)
{
	if ( (ConstraintIndex < 0) || (ConstraintIndex >= ConstraintSetup.Num()) )
	{
		return NAME_None;
	}

	return ConstraintSetup[ConstraintIndex]->DefaultInstance.JointName;
}

void UPhysicsAsset::GetBodyIndicesBelow(TArray<int32>& OutBodyIndices, FName InBoneName, USkeletalMesh* SkelMesh)
{
	int32 BaseIndex = SkelMesh->RefSkeleton.FindBoneIndex(InBoneName);

	// Iterate over all other bodies, looking for 'children' of this one
	for(int32 i=0; i<BodySetup.Num(); i++)
	{
		UBodySetup* BS = BodySetup[i];
		FName TestName = BS->BoneName;
		int32 TestIndex = SkelMesh->RefSkeleton.FindBoneIndex(TestName);

		// We want to return this body as well.
		if(TestIndex == BaseIndex || SkelMesh->RefSkeleton.BoneIsChildOf(TestIndex, BaseIndex))
		{
			OutBodyIndices.Add(i);
		}
	}
}

void UPhysicsAsset::ClearAllPhysicsMeshes()
{
	for(int32 i=0; i<BodySetup.Num(); i++)
	{
		BodySetup[i]->ClearPhysicsMeshes();
	}
}

#if WITH_EDITOR

void UPhysicsAsset::InvalidateAllPhysicsMeshes()
{
	for(int32 i=0; i<BodySetup.Num(); i++)
	{
		BodySetup[i]->InvalidatePhysicsData();
	}
}


void UPhysicsAsset::PostEditUndo()
{
	Super::PostEditUndo();
	
	UpdateBodySetupIndexMap();
	UpdateBoundsBodiesArray();
}

#endif

//// THUMBNAIL SUPPORT //////

/** 
 * Returns a one line description of an object for viewing in the thumbnail view of the generic browser
 */
FString UPhysicsAsset::GetDesc()
{
	return FString::Printf( TEXT("%d Bodies, %d Constraints"), BodySetup.Num(), ConstraintSetup.Num() );
}

void UPhysicsAsset::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	OutTags.Add( FAssetRegistryTag("Bodies", FString::FromInt(BodySetup.Num()), FAssetRegistryTag::TT_Numerical) );
	OutTags.Add( FAssetRegistryTag("Constraints", FString::FromInt(ConstraintSetup.Num()), FAssetRegistryTag::TT_Numerical) );

	Super::GetAssetRegistryTags(OutTags);
}
