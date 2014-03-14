// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Abstract base class of animation assets that can be played back and evaluated to produce a pose.
 *
 */

#pragma once

#include "SkeletalMeshTypes.h"
#include "AnimInterpFilter.h"
#include "AnimationAsset.generated.h"

class USkeleton;
class USkeletalMesh;

/** Transform definition */
USTRUCT(BlueprintType)
struct FBlendSampleData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	int32 SampleDataIndex;

	UPROPERTY()
	float TotalWeight;

	UPROPERTY()
	float Time;

	// transient perbone interpolation data
	TArray<float> PerBoneBlendData;

	FBlendSampleData()
		:	SampleDataIndex(0)
		,	TotalWeight(0.f)
		,	Time(0.f)
	{}
	FBlendSampleData(int32 Index)
		:	SampleDataIndex(Index)
		,	TotalWeight(0.f)
		,	Time(0.f)
	{}
	bool operator==( const FBlendSampleData& Other ) const 
	{
		// if same position, it's same point
		return (Other.SampleDataIndex== SampleDataIndex);
	}
	void AddWeight(float Weight)
	{
		TotalWeight += Weight;
	}
	float GetWeight() const
	{
		return FMath::Clamp<float>(TotalWeight, 0.f, 1.f);
	}
};

USTRUCT()
struct FBlendFilter
{
	GENERATED_USTRUCT_BODY()

	FFIRFilterTimeBased FilterPerAxis[3];

	FBlendFilter()
	{
	}

	FVector GetFilterLastOutput()
	{
		return FVector (FilterPerAxis[0].LastOutput, FilterPerAxis[1].LastOutput, FilterPerAxis[2].LastOutput);
	}
};

// Root Bone Lock options when extracting Root Motion
UENUM()
namespace ERootMotionRootLock
{
	enum Type
	{
		// Use reference pose root bone position
		RefPose,

		// Use root bone position on first frame of animation.
		AnimFirstFrame,

		// FTransform::Identity
		Zero
	};
}


/** Animation Extraction Context */
USTRUCT()
struct FAnimExtractContext
{
	GENERATED_USTRUCT_BODY()

	/** Is Root Motion Translation being extracted? */
	UPROPERTY()
	bool bExtractRootMotionTranslation;

	/** Is Root Motion Rotation being extracted? */
	UPROPERTY()
	bool bExtractRootMotionRotation;

	/** Are we looping this animation? */
	UPROPERTY()
	bool bLooping;

	/** Position in animation to extract pose from */
	UPROPERTY()
	float CurrentTime;

	/** Root Motion Root Bone Lock option. **/
	UPROPERTY()
	TEnumAsByte<ERootMotionRootLock::Type> RootMotionRootLock;

	FAnimExtractContext()
		: bExtractRootMotionTranslation(false)
		, bExtractRootMotionRotation(false)
		, bLooping(false)
		, CurrentTime(0.f)
		, RootMotionRootLock(ERootMotionRootLock::RefPose)
	{
	}

	FAnimExtractContext(float InCurrentTime, bool InbLooping)
		: bExtractRootMotionTranslation(false)
		, bExtractRootMotionRotation(false)
		, bLooping(InbLooping)
		, CurrentTime(InCurrentTime)
		, RootMotionRootLock(ERootMotionRootLock::RefPose)
	{
	}

	FAnimExtractContext(float InCurrentTime, bool InbLooping, bool InbExtractRootMotionTranslation, bool InbExtractRootMotionRotation, ERootMotionRootLock::Type InRootMotionRootLock)
		: bExtractRootMotionTranslation(InbExtractRootMotionTranslation)
		, bExtractRootMotionRotation(InbExtractRootMotionRotation)
		, bLooping(InbLooping)
		, CurrentTime(InCurrentTime)
		, RootMotionRootLock(InRootMotionRootLock)
	{
	}
};

/** Remapping Struct. 
 * Used to remap animation data made for a specific Skeleton, 
 * to another Asset for blending and rendering, typically a SkeletalMesh. */
struct FSkeletonRemappedBoneArray
{
	/** Mapping table between Skeleton Bone Indices and Pose Bone Indices. */
	TArray<int32> SkeletonToPoseBoneIndexArray;

	FSkeletonRemappedBoneArray()
	{
	}

	/**
	 * Serializes the bones
	 *
	 * @param Ar - The archive to serialize into.
	 * @param Rect - The bone container to serialize.
	 *
	 * @return Reference to the Archive after serialization.
	 */
	friend FArchive& operator<<(FArchive& Ar, FSkeletonRemappedBoneArray& B)
	{ 
		Ar << B.SkeletonToPoseBoneIndexArray;	
		return Ar;
	}
};

/**
 * This is a native transient structure.
 * Contains:
 * - BoneIndicesArray: Array of RequiredBoneIndices for Current Asset. In increasing order. Mapping to current Array of Transforms (Pose).
 * - BoneSwitchArray: Size of current Skeleton. true if Bone is contained in RequiredBones array, false otherwise.
 **/
struct FBoneContainer
{
private:
	/** Array of RequiredBonesIndices. In increasing order. */
	TArray<FBoneIndexType>	BoneIndicesArray;
	/** Array sized by Current RefPose. true if Bone is contained in RequiredBones array, false otherwise. */
	TBitArray<>				BoneSwitchArray;

	/** Asset BoneIndicesArray was made for. Typically a SkeletalMesh. */
	TWeakObjectPtr<UObject>	Asset;
	/** If Asset is a SkeletalMesh, this will be a pointer to it. Can be NULL if Asset is a USkeleton. */
	TWeakObjectPtr<USkeletalMesh> AssetSkeletalMesh;
	/** If Asset is a Skeleton that will be it. If Asset is a SkeletalMesh, that will be its Skeleton. */
	TWeakObjectPtr<USkeleton> AssetSkeleton;

	/** Pointer to RefSkeleton of Asset. */
	const FReferenceSkeleton* RefSkeleton;

	/** Array of Pose to Skeleton BoneIndex map. Use to Remap and Retarget current Asset to any Skeleton used by Animations. */
	TArray<FSkeletonRemappedBoneArray> RemappedArrays;

	/** TMap to quickly look up proper map given TargetAsset. */
	TMap<TWeakObjectPtr<class UObject>, int32> AssetToIndexMap;

	/** For debugging. */
	/** Disable Retargeting. Extract animation, but do not retarget it. */
	bool bDisableRetargeting;
	/** Disable animation compression, use RAW data instead. */
	bool bUseRAWData;

public:

	FBoneContainer()
		: Asset(NULL)
		, AssetSkeletalMesh(NULL)
		, AssetSkeleton(NULL)
		, RefSkeleton(NULL)
		, bDisableRetargeting(false)
		, bUseRAWData(false)
	{
		BoneIndicesArray.Empty();
		BoneSwitchArray.Empty();
		RemappedArrays.Empty();
		AssetToIndexMap.Empty();
	}

	FBoneContainer(const TArray<FBoneIndexType>& InRequiredBoneIndexArray, UObject& InAsset)
		: BoneIndicesArray(InRequiredBoneIndexArray)
		, Asset(&InAsset)
		, AssetSkeletalMesh(NULL)
		, AssetSkeleton(NULL)
		, RefSkeleton(NULL)
		, bDisableRetargeting(false)
		, bUseRAWData(false)
	{
		Initialize();
	}

	/** Initialize BoneContainer to a new Asset, RequiredBonesArray and RefPoseArray. */
	void InitializeTo(const TArray<FBoneIndexType>& InRequiredBoneIndexArray, UObject& InAsset);

	/** Returns true if FBoneContainer is Valid. Needs an Asset, a RefPoseArray, and a RequiredBonesArray. */
	const bool IsValid() const
	{
		return (Asset.IsValid() && (RefSkeleton != NULL) && (BoneIndicesArray.Num() > 0));
	}

	/** Get Asset this BoneContainer was made for. Typically a SkeletalMesh, but could also be a USkeleton. */
	UObject* GetAsset() const
	{
		return Asset.Get();
	}

	/** Get SkeletalMesh Asset this BoneContainer was made for. Could be NULL if Asset is a Skeleton. */
	USkeletalMesh* GetSkeletalMeshAsset() const
	{
		return AssetSkeletalMesh.Get();
	}

	/** Get Skeleton Asset. Could either be the SkeletalMesh's Skeleton, or the Skeleton this BoneContainer was made for. Is non NULL is BoneContainer is valid. */
	USkeleton* GetSkeletonAsset() const
	{
		return AssetSkeleton.Get();
	}

	/** Disable Retargeting for debugging. */
	void SetDisableRetargeting(bool InbDisableRetargeting)
	{
		bDisableRetargeting = InbDisableRetargeting;
	}

	/** True if retargeting is disabled for debugging. */
	bool GetDisableRetargeting() const
	{
		return bDisableRetargeting;
	}

	/** Ignore compressed data and use RAW data instead, for debugging. */
	void SetUseRAWData(bool InbUseRAWData)
	{
		bUseRAWData = InbUseRAWData;
	}

	/** True if we're requesting RAW data instead of compressed data. For debugging. */
	bool ShouldUseRawData() const
	{
		return bUseRAWData;
	}

 	/**
	 * returns Required Bone Indices Array
	 */
	const TArray<FBoneIndexType>& GetBoneIndicesArray() const
	{
		return BoneIndicesArray;
	}

	/**
	 * returns Bone Switch Array. BitMask for RequiredBoneIndex array.
	 */
	const TBitArray<>& GetBoneSwitchArray() const
	{
		return BoneSwitchArray;
	}

	/** Pointer to RefPoseArray for current Asset. */
	const TArray<FTransform>& GetRefPoseArray() const
	{
		return RefSkeleton->GetRefBonePose();
	}

	/** Access to Asset's RefSkeleton. */
	const FReferenceSkeleton& GetReferenceSkeleton() const
	{
		return *RefSkeleton;
	}

	/** Number of Bones in RefPose for current asset. This is NOT the number of bones in RequiredBonesArray, but the TOTAL number of bones in the RefPose of the current Asset! */ 
	const int32 GetNumBones() const
	{
		return RefSkeleton->GetNum();
	}

	/** Get BoneIndex for BoneName for current Asset. */
	ENGINE_API int32 GetPoseBoneIndexForBoneName(const FName& BoneName) const;

	/** Get ParentBoneIndex for current Asset. */
	ENGINE_API int32 GetParentBoneIndex(const int32& BoneIndex) const;

	/** Get Depth between bones for current asset. */
	int32 GetDepthBetweenBones(const int32& BoneIndex, const int32& ParentBoneIndex) const;

	/** Returns true if bone is child of for current asset. */
	bool BoneIsChildOf(const int32& BoneIndex, const int32& ParentBoneIndex) const;

	/**
	 * Serializes the bones
	 *
	 * @param Ar - The archive to serialize into.
	 * @param Rect - The bone container to serialize.
	 *
	 * @return Reference to the Archive after serialization.
	 */
	friend FArchive& operator<<( FArchive& Ar, FBoneContainer& B )
	{ 
		Ar 
			<< B.BoneIndicesArray 
			<< B.BoneSwitchArray 
			<< B.Asset 
			<< B.AssetSkeletalMesh 
			<< B.AssetSkeleton 
			<< B.RemappedArrays 
			<< B.AssetToIndexMap
			<< B.bDisableRetargeting
			<< B.bUseRAWData
			;	
		return Ar;
	}

	/**
	 * Returns true of RequiredBonesArray contains this bone index.
	 */
	bool Contains(FBoneIndexType NewIndex) const
	{
		return BoneSwitchArray[NewIndex];
	}

	/** Get Remapping Array between current Asset and requested TargetSkeleton. Used by animation decompression to remap animation data to current asset. */
	const FSkeletonRemappedBoneArray* GetRemappedArrayForSkeleton(USkeleton& TargetSkeleton) const;

private:
	/** Initialize FBoneContainer. */
	ENGINE_API void Initialize();

	/** Cache remapping data if current Asset is a SkeletalMesh, with all compatible Skeletons. */
	const FSkeletonRemappedBoneArray* RemapFromSkelMesh(const USkeletalMesh& SourceSkeletalMesh, USkeleton& TargetSkeleton);

	/** Cache remapping data if current Asset is a Skeleton, with all compatible Skeletons. */
	const FSkeletonRemappedBoneArray* RemapFromSkeleton(const USkeleton& SourceSkeleton, USkeleton& TargetSkeleton);
};

USTRUCT()
struct FBoneReference
{
	GENERATED_USTRUCT_BODY()

	/** Name of bone to control. This is the main bone chain to modify from. **/
	UPROPERTY(EditAnywhere, Category=BoneReference)
	FName BoneName;

	/** Cached bone index for run time - right now bone index of skeleton **/
	int32 BoneIndex;

	FBoneReference()
		: BoneIndex(INDEX_NONE)
	{
	}

	bool operator==( const FBoneReference& Other ) const
	{
		// faster to compare, and BoneName won't matter
		return BoneIndex==Other.BoneIndex;
	}
	/** Initialize Bone Reference, return TRUE if success, otherwise, return false **/
	ENGINE_API bool Initialize(const FBoneContainer& RequiredBones);

	// @fixme laurent - only used by blendspace 'PerBoneBlend'. Fix this to support SkeletalMesh pose.
	ENGINE_API bool Initialize(const USkeleton* Skeleton);

	/** return true if valid. Otherwise return false **/
	ENGINE_API bool IsValid(const FBoneContainer& RequiredBones) const;
};
/**
 * Information about an animation asset that needs to be ticked
 */
USTRUCT()
struct FAnimTickRecord
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	class UAnimationAsset* SourceAsset;

	float* TimeAccumulator;
	FVector BlendSpacePosition;	
	FBlendFilter* BlendFilter;
	TArray<FBlendSampleData>* BlendSampleDataCache;
	float PlayRateMultiplier;
	float EffectiveBlendWeight;
	bool bLooping;

public:
	FAnimTickRecord()
	{
	}
};

UENUM()
namespace EAnimGroupRole
{
	enum Type
	{
		// This node can be the leader, as long as it has a higher blend weight than the previous best leader
		CanBeLeader,
		
		// This node will always be a follower (unless there are only followers, in which case the first one ticked wins)
		AlwaysFollower,

		// This node will always be a leader (if more than one node is AlwaysLeader, the last one ticked wins)
		AlwaysLeader
	};
}

USTRUCT()
struct FAnimGroupInstance
{
	GENERATED_USTRUCT_BODY()

public:
	// The list of animation players in this group which are going to be evaluated this frame
	TArray<FAnimTickRecord> ActivePlayers;

	// The current group leader
	int32 GroupLeaderIndex;
public:
	FAnimGroupInstance()
		: GroupLeaderIndex(INDEX_NONE)
	{
	}

	void Reset()
	{
		GroupLeaderIndex = INDEX_NONE;
		ActivePlayers.Empty(ActivePlayers.Num());
	}

	// Checks the last tick record in the ActivePlayers array to see if it's a better leader than the current candidate.
	// This should be called once for each record added to ActivePlayers, after the record is setup.
	void TestTickRecordForLeadership(EAnimGroupRole::Type MembershipType)
	{
		int32 TestIndex = ActivePlayers.Num() - 1;
		const FAnimTickRecord& Candidate = ActivePlayers[TestIndex];
		
		switch (MembershipType)
		{
		case EAnimGroupRole::CanBeLeader:
			// Set it if we're better than the current leader (or if there is no leader yet)
			if ((GroupLeaderIndex == INDEX_NONE) || (ActivePlayers[GroupLeaderIndex].EffectiveBlendWeight < Candidate.EffectiveBlendWeight))
			{
				// This is a better leader
				GroupLeaderIndex = TestIndex;
			}
			break;
		case EAnimGroupRole::AlwaysLeader:
			// Always set the leader index
			GroupLeaderIndex = TestIndex;
			break;
		default:
		case EAnimGroupRole::AlwaysFollower:
			// Never set the leader index; the actual tick code will handle the case of no leader by using the first element in the array
			break;
		}
	}
};

// This structure is used to either advance or synchronize animation players
struct FAnimAssetTickContext
{
public:
	FAnimAssetTickContext(float InDeltaTime)
		: DeltaTime(InDeltaTime)
		, SyncPoint(0.0f)
		, bIsLeader(true)
	{
	}
	
	// Are we the leader of our sync group (or ungrouped)?
	bool IsLeader() const
	{
		return bIsLeader;
	}

	bool IsFollower() const
	{
		return !bIsLeader;
	}

	// Return the delta time of the tick
	float GetDeltaTime() const
	{
		return DeltaTime;
	}

	void SetSyncPoint(float NormalizedTime)
	{
		SyncPoint = NormalizedTime;
	}

	// Returns the synchronization point (normalized time; only legal to call if ticking a follower)
	float GetSyncPoint() const
	{
		checkSlow(!bIsLeader);
		return SyncPoint;
	}

	void ConvertToFollower()
	{
		bIsLeader = false;
	}

	bool ShouldGenerateNotifies() const
	{
		return IsLeader();
	}

private:
	float DeltaTime;

	// The structure used to pass synchronization state between members of a sync group
	float SyncPoint;

	bool bIsLeader;
};

USTRUCT()
struct FAnimationGroupReference
{
	GENERATED_USTRUCT_BODY()
	
	// The name of the group
	UPROPERTY(EditAnywhere, Category=Settings)
	FName GroupName;

	// The type of membership in the group (potential leader, always follower, etc...)
	UPROPERTY(EditAnywhere, Category=Settings)
	TEnumAsByte<EAnimGroupRole::Type> GroupRole;

	FAnimationGroupReference()
		: GroupRole(EAnimGroupRole::CanBeLeader)
	{
	}
};

UCLASS(abstract, MinimalAPI)
class UAnimationAsset : public UObject
{
	GENERATED_UCLASS_BODY()

private:
	/** Pointer to the Skeleton this asset can be played on .	*/
	UPROPERTY(AssetRegistrySearchable, Category=Animation, VisibleAnywhere, AssetRegistrySearchable)
	class USkeleton* Skeleton;

	/** Skeleton guid. If changes, you need to remap info*/
	FGuid SkeletonGuid;

public:
	/** Advances the asset player instance 
	 * 
	 * @param Instance		AnimationTickRecord Instance - saves data to evaluate
	 * @param InstanceOwner	AnimInstance playing this asset
	 * @param Context		The tick context (leader/follower, delta time, sync point, etc...)
	 */
	virtual void TickAssetPlayerInstance(const FAnimTickRecord& Instance, class UAnimInstance* InstanceOwner, FAnimAssetTickContext& Context) const {}
	// this is used in editor only when used for transition getter
	// this doesn't mean max time. In Sequence, this is SequenceLength,
	// but for BlendSpace CurrentTime is normalized [0,1], so this is 1
	virtual float GetMaxCurrentTime() { return 0.f; }

	ENGINE_API void SetSkeleton(USkeleton* NewSkeleton);
	void ResetSkeleton(USkeleton* NewSkeleton);
	virtual void PostLoad() OVERRIDE;
	virtual void Serialize(FArchive& Ar) OVERRIDE;

#if WITH_EDITOR
	/** Replace Skeleton 
	 * 
	 * @param NewSkeleton	NewSkeleton to change to 
	 */
	ENGINE_API bool ReplaceSkeleton(USkeleton* NewSkeleton);

	/** Retrieve all animations that are used by this asset 
	 * 
	 * @param (out)		AnimationSequences 
	 **/
	ENGINE_API virtual bool GetAllAnimationSequencesReferred(TArray<class UAnimSequence*>& AnimationSequences);

	/** Replace this assets references to other animations based on ReplacementMap 
	 * 
	 * @param ReplacementMap	Mapping of original asset to new asset
	 **/
	ENGINE_API virtual void ReplaceReferredAnimations(const TMap<UAnimSequence*, UAnimSequence*>& ReplacementMap);	

#endif

public:
	class USkeleton* GetSkeleton() const { return Skeleton; }
};

