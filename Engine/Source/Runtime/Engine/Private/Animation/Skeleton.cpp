// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	UnSkeletalAnim.cpp: Skeletal mesh animation functions.
=============================================================================*/ 

#include "EnginePrivate.h"
#include "AssetRegistryModule.h"

#define LOCTEXT_NAMESPACE "Skeleton"
#define ROOT_BONE_PARENT	INDEX_NONE

#if WITH_EDITOR
const FName USkeleton::AnimNotifyTag = FName(TEXT("AnimNotifyList"));
const TCHAR USkeleton::AnimNotifyTagDeliminator = TEXT(';');
#endif 

USkeleton::USkeleton(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void USkeleton::PostInitProperties()
{
	Super::PostInitProperties();

	// this gets called after constructor, and this data can get
	// serialized back if this already has Guid
	if (!IsTemplate())
	{
		Guid = FGuid::NewGuid();
	}
}

void USkeleton::PostLoad()
{
	Super::PostLoad();

	if( GetLinker() && (GetLinker()->UE4Ver() < VER_UE4_REFERENCE_SKELETON_REFACTOR) )
	{
		// Convert RefLocalPoses & BoneTree to FReferenceSkeleton
		ConvertToFReferenceSkeleton();
	}

	// catch any case if guid isn't valid
	check(Guid.IsValid());

#if WITH_EDITOR
	CollectAnimationNotifies();
#endif
}

void USkeleton::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (!bDuplicateForPIE)
	{
		// regenerate Guid
		Guid = FGuid::NewGuid();

		// catch any case if guid isn't valid
		check(Guid.IsValid());
	}
}

void USkeleton::Serialize( FArchive& Ar )
{
	Super::Serialize(Ar);

	if( Ar.UE4Ver() >= VER_UE4_REFERENCE_SKELETON_REFACTOR )
	{
		Ar << ReferenceSkeleton;
	}

	if (Ar.UE4Ver() >= VER_UE4_FIX_ANIMATIONBASEPOSE_SERIALIZATION)
	{
		// Load Animation RetargetSources
		if (Ar.IsLoading())
		{
			int32 NumOfRetargetSources;
			Ar << NumOfRetargetSources;

			FName RetargetSourceName;
			FReferencePose RetargetSource;
			for (int32 Index=0; Index<NumOfRetargetSources; ++Index)
			{
				Ar << RetargetSourceName;
				Ar << RetargetSource;

				AnimRetargetSources.Add(RetargetSourceName, RetargetSource);
			}
		}
		else 
		{
			int32 NumOfRetargetSources = AnimRetargetSources.Num();
			Ar << NumOfRetargetSources;

			for (auto Iter = AnimRetargetSources.CreateIterator(); Iter; ++Iter)
			{
				Ar << Iter.Key();
				Ar << Iter.Value();
			}
		}
	}
	else
	{
		// this is broken, but we have to keep it to not corrupt content. 
		for (auto Iter = AnimRetargetSources.CreateIterator(); Iter; ++Iter)
		{
			Ar << Iter.Key();
			Ar << Iter.Value();
		}
	}

	if (Ar.UE4Ver() < VER_UE4_SKELETON_GUID_SERIALIZATION)
	{
		Guid = FGuid::NewGuid();
	}
	else
	{
		Ar << Guid;
	}
}

/** Remove this function when VER_UE4_REFERENCE_SKELETON_REFACTOR is removed. */
void USkeleton::ConvertToFReferenceSkeleton()
{
	check( BoneTree.Num() == RefLocalPoses_DEPRECATED.Num() );

	const int32 NumRefBones = RefLocalPoses_DEPRECATED.Num();
	ReferenceSkeleton.Empty();

	for(int32 BoneIndex=0; BoneIndex<NumRefBones; BoneIndex++)
	{
		const FBoneNode & BoneNode = BoneTree[BoneIndex];
		FMeshBoneInfo BoneInfo(BoneNode.Name_DEPRECATED, BoneNode.ParentIndex_DEPRECATED);
		const FTransform & BoneTransform = RefLocalPoses_DEPRECATED[BoneIndex];

		// All should be good. Parents before children, no duplicate bones?
		ReferenceSkeleton.Add(BoneInfo, BoneTransform);
	}

	// technically here we should call 	RefershAllRetargetSources(); but this is added after 
	// VER_UE4_REFERENCE_SKELETON_REFACTOR, this shouldn't be needed. It shouldn't have any 
	// AnimatedRetargetSources
	ensure (AnimRetargetSources.Num() == 0);
}

bool USkeleton::DoesParentChainMatch(int32 StartBoneIndex, USkeletalMesh * InSkelMesh) const
{
	const FReferenceSkeleton & SkeletonRefSkel = ReferenceSkeleton;
	const FReferenceSkeleton & MeshRefSkel = InSkelMesh->RefSkeleton;

	// if start is root bone
	if ( StartBoneIndex == 0 )
	{
		// verify name of root bone matches
		return (SkeletonRefSkel.GetBoneName(0) == MeshRefSkel.GetBoneName(0));
	}

	int32 SkeletonBoneIndex = StartBoneIndex;
	// If skeleton bone is not found in mesh, fail.
	int32 MeshBoneIndex = MeshRefSkel.FindBoneIndex( SkeletonRefSkel.GetBoneName(SkeletonBoneIndex) );
	if( MeshBoneIndex == INDEX_NONE )
	{
		return false;
	}
	do
	{
		// verify if parent name matches
		int32 ParentSkeletonBoneIndex = SkeletonRefSkel.GetParentIndex(SkeletonBoneIndex);
		int32 ParentMeshBoneIndex = MeshRefSkel.GetParentIndex(MeshBoneIndex);

		// if one of the parents doesn't exist, make sure both end. Otherwise fail.
		if( (ParentSkeletonBoneIndex == INDEX_NONE) || (ParentMeshBoneIndex == INDEX_NONE) )
		{
			return (ParentSkeletonBoneIndex == ParentMeshBoneIndex);
		}

		// If parents are not named the same, fail.
		if( SkeletonRefSkel.GetBoneName(ParentSkeletonBoneIndex) != MeshRefSkel.GetBoneName(ParentMeshBoneIndex) )
		{
			return false;
		}

		// move up
		SkeletonBoneIndex = ParentSkeletonBoneIndex;
		MeshBoneIndex = ParentMeshBoneIndex;
	} while ( true );

	return true;
}

bool USkeleton::IsCompatibleMesh(USkeletalMesh* InSkelMesh) const
{
	// at least % of bone should match 
	int32 NumOfBoneMatches = 0;

	const FReferenceSkeleton & SkeletonRefSkel = ReferenceSkeleton;
	const FReferenceSkeleton & MeshRefSkel = InSkelMesh->RefSkeleton;
	const int32 NumBones = MeshRefSkel.GetNum();

	// first ensure the parent exists for each bone
	for (int32 MeshBoneIndex=0; MeshBoneIndex<NumBones; MeshBoneIndex++)
	{
		// See if Mesh bone exists in Skeleton.
		int32 SkeletonBoneIndex = SkeletonRefSkel.FindBoneIndex( MeshRefSkel.GetBoneName(MeshBoneIndex) );

		// if found, increase num of bone matches count
		if( SkeletonBoneIndex != INDEX_NONE )
		{
			++NumOfBoneMatches;
		}
		else
		{
			int32 CurrentBoneId = MeshBoneIndex;
			// if not look for parents that matches
			while (SkeletonBoneIndex == INDEX_NONE && CurrentBoneId != INDEX_NONE)
			{
				// find Parent one see exists
				const int32 ParentMeshBoneIndex = MeshRefSkel.GetParentIndex(CurrentBoneId);
				if ( ParentMeshBoneIndex != INDEX_NONE )
				{
					// @TODO: make sure RefSkeleton's root ParentIndex < 0 if not, I'll need to fix this by checking TreeBoneIdx
					SkeletonBoneIndex = SkeletonRefSkel.FindBoneIndex(MeshRefSkel.GetBoneName(ParentMeshBoneIndex));
				}

				// root is reached
				if( ParentMeshBoneIndex == 0 )
				{
					break;
				}
				else
				{
					CurrentBoneId = ParentMeshBoneIndex;
				}
			}

			// still no match, return false, no parent to look for
			if( SkeletonBoneIndex == INDEX_NONE )
			{
				return false;
			}

			// second follow the parent chain to verify the chain is same
			if( !DoesParentChainMatch(SkeletonBoneIndex, InSkelMesh) )
			{
				return false;
			}
		}
	}

	// originally we made sure at least matches more than 50% 
	// but then slave components can't play since they're only partial
	// if the hierarchy matches, and if it's more then 1 bone, we allow
	return (NumOfBoneMatches > 0);
}

FString USkeleton::GetRetargetingModeString(const EBoneTranslationRetargetingMode::Type & RetargetingMode) const
{
	FText ModeNameText;

	switch( RetargetingMode )
	{
	case EBoneTranslationRetargetingMode::Animation :
		ModeNameText = LOCTEXT( "BoneRetargetingModeAnimation", "Animation" );
		break;

	case EBoneTranslationRetargetingMode::Skeleton :
		ModeNameText = LOCTEXT( "BoneRetargetingModeSkeleton", "Skeleton" );
		break;

	case EBoneTranslationRetargetingMode::AnimationScaled :
		ModeNameText = LOCTEXT( "BoneRetargetingMode", "AnimationScaled" );
		break;

	default:
		// Unknown mode
		check( 0 );
		break;
	}

	return ModeNameText.ToString();
}

void USkeleton::ClearCacheData()
{
	LinkupCache.Empty();
	SkelMesh2LinkupCache.Empty();
}


int32 USkeleton::GetMeshLinkupIndex(const USkeletalMesh * InSkelMesh)
{
	const int32* IndexPtr = SkelMesh2LinkupCache.Find(InSkelMesh);
	int32 LinkupIndex = INDEX_NONE;

	if ( IndexPtr == NULL )
	{
		LinkupIndex = BuildLinkup(InSkelMesh);
	}
	else
	{
		LinkupIndex = *IndexPtr;
	}

	// make sure it's not out of range
	check (LinkupIndex < LinkupCache.Num());

	return LinkupIndex;
}


int32 USkeleton::BuildLinkup(const USkeletalMesh * InSkelMesh)
{
	const FReferenceSkeleton & SkeletonRefSkel = ReferenceSkeleton;
	const FReferenceSkeleton & MeshRefSkel = InSkelMesh->RefSkeleton;

	// @todoanim : need to refresh NULL SkeletalMeshes from Cache
	// since now they're autoweak pointer, they will go away if not used
	// so whenever map transition happens, this links will need to clear up
	FSkeletonToMeshLinkup NewMeshLinkup;

	const int32 NumSkeletonBones = SkeletonRefSkel.GetNum();
	NewMeshLinkup.SkeletonToMeshTable.Empty(NumSkeletonBones);
	NewMeshLinkup.SkeletonToMeshTable.AddUninitialized(NumSkeletonBones);
	
	for (int32 SkeletonBoneIndex=0; SkeletonBoneIndex<NumSkeletonBones; SkeletonBoneIndex++)
	{
		const int32 MeshBoneIndex = MeshRefSkel.FindBoneIndex( SkeletonRefSkel.GetBoneName(SkeletonBoneIndex) );
		NewMeshLinkup.SkeletonToMeshTable[SkeletonBoneIndex] = MeshBoneIndex;
	}

	// adding the other direction mapping table
	// since now we're planning to use Skeleton.BoneTree to be main RequiredBones,
	// we'll need a lot of conversion back/forth between ref pose and skeleton
	const int32 NumMeshBones = MeshRefSkel.GetNum();
	NewMeshLinkup.MeshToSkeletonTable.Empty(NumMeshBones);
	NewMeshLinkup.MeshToSkeletonTable.AddUninitialized(NumMeshBones);

	for (int32 MeshBoneIndex=0; MeshBoneIndex<NumMeshBones; MeshBoneIndex++)
	{
		const int32 SkeletonBoneIndex = SkeletonRefSkel.FindBoneIndex( MeshRefSkel.GetBoneName(MeshBoneIndex) );
		NewMeshLinkup.MeshToSkeletonTable[MeshBoneIndex] = SkeletonBoneIndex;
	}

	int32 NewIndex = LinkupCache.Add(NewMeshLinkup);
	check (NewIndex != INDEX_NONE);
	SkelMesh2LinkupCache.Add(InSkelMesh, NewIndex);

	return NewIndex;
}


void USkeleton::RebuildLinkup(const USkeletalMesh * InSkelMesh)
{
	// remove the key
	SkelMesh2LinkupCache.Remove(InSkelMesh);
	// build new one
	BuildLinkup(InSkelMesh);
}

void USkeleton::UpdateReferencePoseFromMesh(const USkeletalMesh * InSkelMesh)
{
	check (InSkelMesh);

	for (int32 BoneIndex=0; BoneIndex<ReferenceSkeleton.GetNum(); BoneIndex++)
	{
		// find index from ref pose array
		const int32 MeshBoneIndex = InSkelMesh->RefSkeleton.FindBoneIndex(ReferenceSkeleton.GetBoneName(BoneIndex));
		if( MeshBoneIndex != INDEX_NONE )
		{
			ReferenceSkeleton.UpdateRefPoseTransform(BoneIndex, InSkelMesh->RefSkeleton.GetRefBonePose()[MeshBoneIndex]);
		}
	}

	MarkPackageDirty();
}

bool USkeleton::RecreateBoneTree(USkeletalMesh* InSkelMesh)
{
	if( InSkelMesh )
	{
		BoneTree.Empty();
		ReferenceSkeleton.Empty();
		return MergeAllBonesToBoneTree(InSkelMesh);
	}
	return false;
}

bool USkeleton::MergeAllBonesToBoneTree(USkeletalMesh * InSkelMesh)
{
	if( InSkelMesh )
	{
		TArray<int32> RequiredBoneIndices;

		// for now add all in this case. 
		RequiredBoneIndices.AddUninitialized(InSkelMesh->RefSkeleton.GetNum());
		// gather bone list
		for (int32 I=0; I<InSkelMesh->RefSkeleton.GetNum(); ++I)
		{
			RequiredBoneIndices[I] = I;
		}

		if( RequiredBoneIndices.Num() > 0 )
		{
			// merge bones to the selected skeleton
			return MergeBonesToBoneTree( InSkelMesh, RequiredBoneIndices );
		}
	}

	return false;
}

bool USkeleton::CreateReferenceSkeletonFromMesh(const USkeletalMesh * InSkeletalMesh, const TArray<int32> & RequiredRefBones)
{
	// Filter list, we only want bones that have their parents present in this array.
	TArray<int32> FilteredRequiredBones; 
	FAnimationRuntime::ExcludeBonesWithNoParents(RequiredRefBones, InSkeletalMesh->RefSkeleton, FilteredRequiredBones);

	if( FilteredRequiredBones.Num() > 0 )
	{
		const int32 NumBones = FilteredRequiredBones.Num();
		ReferenceSkeleton.Allocate(NumBones);
		BoneTree.Empty(NumBones);
		BoneTree.AddZeroed(NumBones);

		for (int32 Index=0; Index<FilteredRequiredBones.Num(); Index++)
		{
			const int32 & BoneIndex = FilteredRequiredBones[Index];

			FMeshBoneInfo NewMeshBoneInfo = InSkeletalMesh->RefSkeleton.GetRefBoneInfo()[BoneIndex];
			// Fix up ParentIndex for our new Skeleton.
			if( BoneIndex == 0 )
			{
				NewMeshBoneInfo.ParentIndex = INDEX_NONE; // root
			}
			else
			{
				NewMeshBoneInfo.ParentIndex = ReferenceSkeleton.FindBoneIndex(InSkeletalMesh->RefSkeleton.GetBoneName(InSkeletalMesh->RefSkeleton.GetParentIndex(BoneIndex)));
			}
			ReferenceSkeleton.Add(NewMeshBoneInfo, InSkeletalMesh->RefSkeleton.GetRefBonePose()[BoneIndex]);
		}

		return true;
	}

	return false;
}


bool USkeleton::MergeBonesToBoneTree(USkeletalMesh * InSkeletalMesh, const TArray<int32> & RequiredRefBones)
{
	// see if it needs all animation data to remap - only happens when bone structure CHANGED - added
	bool bSuccess = false;
	// clear cache data since it won't work anymore once this is done
	ClearCacheData();

	// if it's first time
	if( BoneTree.Num() == 0 )
	{
		bSuccess = CreateReferenceSkeletonFromMesh(InSkeletalMesh, RequiredRefBones);
	}
	else
	{
		// can we play? - hierarchy matches
		if( IsCompatibleMesh(InSkeletalMesh) )
		{
			// Exclude bones who do not have a parent.
			TArray<int32> FilteredRequiredBones;
			FAnimationRuntime::ExcludeBonesWithNoParents(RequiredRefBones, InSkeletalMesh->RefSkeleton, FilteredRequiredBones);

			for (int32 Index=0; Index<FilteredRequiredBones.Num(); Index++)
			{
				const int32 & MeshBoneIndex = FilteredRequiredBones[Index];
				const int32 & SkeletonBoneIndex = ReferenceSkeleton.FindBoneIndex(InSkeletalMesh->RefSkeleton.GetBoneName(MeshBoneIndex));
				
				// Bone doesn't already exist. Add it.
				if( SkeletonBoneIndex == INDEX_NONE )
				{
					FMeshBoneInfo NewMeshBoneInfo = InSkeletalMesh->RefSkeleton.GetRefBoneInfo()[MeshBoneIndex];
					// Fix up ParentIndex for our new Skeleton.
					if( ReferenceSkeleton.GetNum() == 0 )
					{
						NewMeshBoneInfo.ParentIndex = INDEX_NONE; // root
					}
					else
					{
						NewMeshBoneInfo.ParentIndex = ReferenceSkeleton.FindBoneIndex(InSkeletalMesh->RefSkeleton.GetBoneName(InSkeletalMesh->RefSkeleton.GetParentIndex(MeshBoneIndex)));
					}

					ReferenceSkeleton.Add(NewMeshBoneInfo, InSkeletalMesh->RefSkeleton.GetRefBonePose()[MeshBoneIndex]);
					BoneTree.AddZeroed(1);
				}
			}

			bSuccess = true;
		}
	}

	// if succeed
	if (bSuccess)
	{
		InSkeletalMesh->Skeleton = this;
		InSkeletalMesh->MarkPackageDirty();
		// make sure to refresh all base poses
		// so that they have same number of bones of ref pose
#if WITH_EDITORONLY_DATA
		RefreshAllRetargetSources();
#endif
	}

	return bSuccess;
}

void USkeleton::SetBoneTranslationRetargetingMode(const int32 & BoneIndex, EBoneTranslationRetargetingMode::Type NewRetargetingMode, bool bChildrenToo)
{
	BoneTree[BoneIndex].TranslationRetargetingMode = NewRetargetingMode;

	if( bChildrenToo )
	{
		// Bones are guaranteed to be sorted in increasing order. So children will be after this bone.
		const int32 NumBones = ReferenceSkeleton.GetNum();
		for(int32 ChildIndex=BoneIndex+1; ChildIndex<NumBones; ChildIndex++)
		{
			if( ReferenceSkeleton.BoneIsChildOf(ChildIndex, BoneIndex) )
			{
				BoneTree[ChildIndex].TranslationRetargetingMode = NewRetargetingMode;
			}
		}
	}
}

int32 USkeleton::GetAnimationTrackIndex(const int32 & InSkeletonBoneIndex, const UAnimSequence * InAnimSeq)
{
	if( InSkeletonBoneIndex != INDEX_NONE )
	{
		for (int32 TrackIndex=0; TrackIndex<InAnimSeq->TrackToSkeletonMapTable.Num(); ++TrackIndex)
		{
			const FTrackToSkeletonMap & TrackToSkeleton = InAnimSeq->TrackToSkeletonMapTable[TrackIndex];
			if( TrackToSkeleton.BoneTreeIndex == InSkeletonBoneIndex )
			{
				return TrackIndex;
			}
		}
	}

	return INDEX_NONE;
}


int32 USkeleton::GetSkeletonBoneIndexFromMeshBoneIndex(const USkeletalMesh * InSkelMesh, const int32 & MeshBoneIndex)
{
	check(MeshBoneIndex != INDEX_NONE);
	const int32 LinkupCacheIdx = GetMeshLinkupIndex(InSkelMesh);
	const FSkeletonToMeshLinkup & LinkupTable = LinkupCache[LinkupCacheIdx];

	return LinkupTable.MeshToSkeletonTable[MeshBoneIndex];
}


int32 USkeleton::GetMeshBoneIndexFromSkeletonBoneIndex(const USkeletalMesh * InSkelMesh, const int32 & SkeletonBoneIndex)
{
	check(SkeletonBoneIndex != INDEX_NONE);
	const int32 LinkupCacheIdx = GetMeshLinkupIndex(InSkelMesh);
	const FSkeletonToMeshLinkup & LinkupTable = LinkupCache[LinkupCacheIdx];

	return LinkupTable.SkeletonToMeshTable[SkeletonBoneIndex];
}

#if WITH_EDITORONLY_DATA
void USkeleton::UpdateRetargetSource( const FName Name )
{
	FReferencePose * PoseFound = AnimRetargetSources.Find(Name);

	if (PoseFound)
	{
		USkeletalMesh * ReferenceMesh = PoseFound->ReferenceMesh;
		
		// reference mesh can be deleted after base pose is created, don't update it if it's not there. 
		if(ReferenceMesh)
		{
			const TArray<FTransform>& MeshRefPose = ReferenceMesh->RefSkeleton.GetRefBonePose();
			const TArray<FTransform>& SkeletonRefPose = GetReferenceSkeleton().GetRefBonePose();
			const TArray<FMeshBoneInfo> & SkeletonBoneInfo = GetReferenceSkeleton().GetRefBoneInfo();

			PoseFound->ReferencePose.Empty(SkeletonRefPose.Num());
			PoseFound->ReferencePose.AddUninitialized(SkeletonRefPose.Num());

			for (int32 SkeletonBoneIndex=0; SkeletonBoneIndex<SkeletonRefPose.Num(); ++SkeletonBoneIndex)
			{
				FName SkeletonBoneName = SkeletonBoneInfo[SkeletonBoneIndex].Name;
				int32 MeshBoneIndex = ReferenceMesh->RefSkeleton.FindBoneIndex(SkeletonBoneName);
				if (MeshBoneIndex != INDEX_NONE)
				{
					PoseFound->ReferencePose[SkeletonBoneIndex] = MeshRefPose[MeshBoneIndex];
				}
				else
				{
					PoseFound->ReferencePose[SkeletonBoneIndex] = FTransform::Identity;
				}
			}
		}
		else
		{
			UE_LOG(LogAnimation, Warning, TEXT("Reference Mesh for Retarget Source %s has been removed."), *Name.ToString());
		}
	}
}

void USkeleton::RefreshAllRetargetSources()
{
	for (auto Iter = AnimRetargetSources.CreateConstIterator(); Iter; ++Iter)
	{
		UpdateRetargetSource(Iter.Key());
	}
}

int32 USkeleton::GetChildBones(int32 ParentBoneIndex, TArray<int32> & Children) const
{
	Children.Empty();

	const int32 NumBones = ReferenceSkeleton.GetNum();
	for(int32 ChildIndex=ParentBoneIndex+1; ChildIndex<NumBones; ChildIndex++)
	{
		if ( ParentBoneIndex == ReferenceSkeleton.GetParentIndex(ChildIndex) )
		{
			Children.Add(ChildIndex);
		}
	}

	return Children.Num();
}

void USkeleton::CollectAnimationNotifies()
{
	// need to verify if these pose is used by anybody else
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	// @Todo : remove it when we know the asset registry is updated
	// meanwhile if you remove this, this will miss the links
	//AnimationNotifies.Empty();
	TArray<FAssetData> AssetList;
	AssetRegistryModule.Get().GetAssetsByClass(UAnimSequenceBase::StaticClass()->GetFName(), AssetList, true);

	// do not clear AnimationNotifies. We can't remove old ones yet. 
	FString CurrentSkeletonName = FAssetData(this).GetExportTextName();
	for (auto Iter = AssetList.CreateConstIterator(); Iter; ++Iter)
	{
		const FAssetData& Asset = *Iter;
		const FString * SkeletonValue = Asset.TagsAndValues.Find(TEXT("Skeleton"));
		if (SkeletonValue && *SkeletonValue == CurrentSkeletonName)
		{
			if (const FString * Value = Asset.TagsAndValues.Find(USkeleton::AnimNotifyTag))
			{
				TArray<FString> NotifyList;
				Value->ParseIntoArray(&NotifyList, &AnimNotifyTagDeliminator, true);
				for (auto NotifyIter = NotifyList.CreateConstIterator(); NotifyIter; ++NotifyIter)
				{
					FString NotifyName = *NotifyIter;
					AddNewAnimationNotify(FName(*NotifyName));
				}
			}
		}
	}
}

void USkeleton::AddNewAnimationNotify(FName NewAnimNotifyName)
{
	if (NewAnimNotifyName!=NAME_None)
	{
		AnimationNotifies.AddUnique( NewAnimNotifyName);
	}
}

USkeletalMesh* USkeleton::GetPreviewMesh()
{
	USkeletalMesh* PreviewMesh = PreviewSkeletalMesh.Get();
	if(!PreviewMesh)
	{
		// if preview mesh isn't loaded, see if we have set
		FStringAssetReference PreviewMeshStringRef = PreviewSkeletalMesh.ToStringReference();
		// load it since now is the time to load
		if(!PreviewMeshStringRef.AssetLongPathname.IsEmpty())
		{
			PreviewMesh = Cast<USkeletalMesh>(StaticLoadObject(USkeletalMesh::StaticClass(), NULL, *PreviewMeshStringRef.AssetLongPathname, NULL, LOAD_None, NULL));
		}
	}
	return PreviewMesh;
}

void USkeleton::SetPreviewMesh(USkeletalMesh* PreviewMesh, bool bMarkAsDirty/*=true*/)
{
	if (bMarkAsDirty)
	{
		Modify();
	}

	PreviewSkeletalMesh = PreviewMesh;
}

int32 USkeleton::ValidatePreviewAttachedObjects()
{
	int32 NumBrokenAssets = 0;

	//Load up attached objects
	for(int32 i = PreviewAttachedAssetContainer.Num()-1; i >= 0; --i)
	{
		FPreviewAttachedObjectPair& PreviewAttachedObject = PreviewAttachedAssetContainer[i];

		if(!PreviewAttachedObject.Object)
		{
			PreviewAttachedAssetContainer.RemoveAtSwap(i);
			++NumBrokenAssets;
		}
	}

	if(NumBrokenAssets > 0)
	{
		MarkPackageDirty();
	}
	return NumBrokenAssets;
}

int32 USkeleton::RemoveBoneFromLOD(int32 LODIndex, int32 BoneIndex)
{
	// we don't remove from LOD 0
	if (!LODIndex)
	{
		return 0;
	}
	
	int32 NumOfLODSetting = BoneReductionSettingsForLODs.Num();
	if (NumOfLODSetting < LODIndex)
	{
		BoneReductionSettingsForLODs.AddZeroed(LODIndex-NumOfLODSetting);
	}

	int32 TotalNumberAdded=0;
	FName BoneName = ReferenceSkeleton.GetBoneName(BoneIndex);
	if ( BoneReductionSettingsForLODs[LODIndex-1].Add(BoneName) )
	{
		++TotalNumberAdded;
	}

	// make sure to add all children
	const int32 NumBones = ReferenceSkeleton.GetNum();
	for(int32 ChildIndex=BoneIndex+1; ChildIndex<NumBones; ChildIndex++)
	{
		if( ReferenceSkeleton.BoneIsChildOf(ChildIndex, BoneIndex) )
		{
			BoneName = ReferenceSkeleton.GetBoneName(ChildIndex);
			if ( BoneReductionSettingsForLODs[LODIndex-1].Add(BoneName) )
			{
				++TotalNumberAdded;
			}
		}
	}

	return TotalNumberAdded;
}

bool USkeleton::IsBoneIncludedInLOD(int32 LODIndex, int32 BoneIndex)
{
	if (LODIndex && BoneReductionSettingsForLODs.Num() >= LODIndex)
	{
		const FName BoneName = ReferenceSkeleton.GetBoneName(BoneIndex);
		return BoneReductionSettingsForLODs[LODIndex-1].Contains(BoneName) == false;
	}

	return true;
}

void USkeleton::AddBoneToLOD(int32 LODIndex, int32 BoneIndex)
{
	if (LODIndex && BoneReductionSettingsForLODs.Num() >= LODIndex)
	{
		const FName BoneName = ReferenceSkeleton.GetBoneName(BoneIndex);
		BoneReductionSettingsForLODs[LODIndex-1].Remove(BoneName);
	}
}

#endif

#undef LOCTEXT_NAMESPACE 