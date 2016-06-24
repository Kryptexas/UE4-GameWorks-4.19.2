// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "AnimationUtils.h"
#include "AnimationRuntime.h"
#include "Animation/PoseAsset.h"


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FPoseDataContainer
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void FPoseDataContainer::Reset()
{
	// clear everything
	PoseNames.Reset();
	Poses.Reset();
	Tracks.Reset();
	TrackMap.Reset();
	Curves.Reset();
}

void FPoseDataContainer::AddOrUpdatePose(const FSmartName& InPoseName, const TArray<FTransform>& InlocalSpacePose, const TArray<float>& InCurveData)
{
	// make sure the transform is correct size
	if (ensureAlways(InlocalSpacePose.Num() == Tracks.Num()))
	{
		// find or add pose data
		FPoseData* PoseDataPtr = FindOrAddPoseData(InPoseName);
		// now add pose
		PoseDataPtr->LocalSpacePose = InlocalSpacePose;
		PoseDataPtr->CurveData = InCurveData;
	}

	// for now we only supports same tracks
}

bool FPoseDataContainer::InsertTrack(const FName& InTrackName, USkeleton* InSkeleton, FName& InRetargetSourceName)
{
	check(InSkeleton);

	// make sure the transform is correct size
	if (Tracks.Contains(InTrackName))
	{
		int32 SkeletonIndex = InSkeleton->GetReferenceSkeleton().FindBoneIndex(InTrackName);
		int32 TrackIndex = INDEX_NONE;
		if (SkeletonIndex != INDEX_NONE)
		{
			Tracks.Add(InTrackName);
			TrackMap.Add(InTrackName, SkeletonIndex);
			TrackIndex = Tracks.Num() - 1;

			// now insert default refpose
			const FTransform DefaultPose = GetDefaultTransform(SkeletonIndex, InSkeleton, InRetargetSourceName);

			for (auto& PoseData : Poses)
			{
				ensureAlways(PoseData.LocalSpacePose.Num() == TrackIndex);

				PoseData.LocalSpacePose.Add(DefaultPose);

				// make sure they always match
				ensureAlways(PoseData.LocalSpacePose.Num() == Tracks.Num());
			}

			return true;
		}

		return false;
	}

	return false;
}

void FPoseDataContainer::GetPoseCurve(const FPoseData* PoseData, FBlendedCurve& OutCurve) const
{
	if (PoseData)
	{
		const TArray<float>& CurveValues = PoseData->CurveData;
		checkSlow(CurveValues.Num() == Curves.Num());

		// extract curve - not optimized, can use optimization
		for (int32 CurveIndex = 0; CurveIndex < Curves.Num(); ++CurveIndex)
		{
			const FAnimCurveBase& Curve = Curves[CurveIndex];
			OutCurve.Set(Curve.Name.UID, CurveValues[CurveIndex], Curve.GetCurveTypeFlags());
		}
	}
}
// remove track if all poses has identity key
// we don't want to save transform per track (good for compression) because that will slow down blending
void FPoseDataContainer::Shrink(USkeleton* InSkeleton, FName& InRetargetSourceName)
{
}

FTransform FPoseDataContainer::GetDefaultTransform(const FName& InTrackName, USkeleton* InSkeleton, const FName& InRetargetSourceName) const
{
	int32 SkeletonIndex = InSkeleton->GetReferenceSkeleton().FindBoneIndex(InTrackName);
	if (SkeletonIndex != INDEX_NONE)
	{
		return GetDefaultTransform(SkeletonIndex, InSkeleton, InRetargetSourceName);
	}

	return FTransform::Identity;
}

FTransform FPoseDataContainer::GetDefaultTransform(int32 SkeletonIndex, USkeleton* InSkeleton, const FName& InRetargetSourceName) const
{
	// now insert default refpose
	const TArray<FTransform>& RefPose = InSkeleton->GetRefLocalPoses(InRetargetSourceName);

	if (RefPose.IsValidIndex(SkeletonIndex))
	{
		return RefPose[SkeletonIndex];
	}

	return FTransform::Identity;
}

bool FPoseDataContainer::FillUpDefaultPose(FPoseData* PoseData, USkeleton* InSkeleton, FName& InRetargetSourceName)
{
	if (PoseData)
	{
		int32 TrackIndex = 0;
		const TArray<FTransform>& RefPose = InSkeleton->GetRefLocalPoses(InRetargetSourceName);
		for (TPair<FName, int32>& TrackIter : TrackMap)
		{
			int32 SkeletonIndex = TrackIter.Value;
			PoseData->LocalSpacePose[TrackIndex] = RefPose[SkeletonIndex];

			++TrackIndex;
		}

		return true;
	}

	return false;
}

bool FPoseDataContainer::FillUpDefaultPose(const FSmartName& InPoseName, USkeleton* InSkeleton, FName& InRetargetSourceName)
{
	const TArray<FTransform>& RefPose = InSkeleton->GetRefLocalPoses(InRetargetSourceName);
	FPoseData* PoseData = FindPoseData(InPoseName);

	if (PoseData)
	{
		return FillUpDefaultPose(PoseData, InSkeleton, InRetargetSourceName);
	}

	return false;
}

FPoseData* FPoseDataContainer::FindPoseData(FSmartName PoseName)
{
	int32 PoseIndex = PoseNames.Find(PoseName);
	if (PoseIndex != INDEX_NONE)
	{
		return &Poses[PoseIndex];
	}

	return nullptr;
}

FPoseData* FPoseDataContainer::FindOrAddPoseData(FSmartName PoseName)
{
	int32 PoseIndex = PoseNames.Find(PoseName);
	if (PoseIndex == INDEX_NONE)
	{
		PoseIndex = PoseNames.Add(PoseName);
		check(PoseIndex == Poses.AddZeroed(1));
	}

	return &Poses[PoseIndex];
}

void FPoseDataContainer::RenamePose(FSmartName OldPoseName, FSmartName NewPoseName)
{
	int32 PoseIndex = PoseNames.Find(OldPoseName);
	if (PoseIndex != INDEX_NONE)
	{
		PoseNames[PoseIndex] = NewPoseName;
	}
}

bool FPoseDataContainer::DeletePose(FSmartName PoseName)
{
	int32 PoseIndex = PoseNames.Find(PoseName);
	if (PoseIndex != INDEX_NONE)
	{
		PoseNames.RemoveAt(PoseIndex);
		Poses.RemoveAt(PoseIndex);
		return true;
	}

	return false;
}

bool FPoseDataContainer::DeleteCurve(FSmartName CurveName)
{
	for (int32 CurveIndex = 0; CurveIndex < Curves.Num(); ++CurveIndex)
	{
		if (Curves[CurveIndex].Name == CurveName)
		{
			Curves.RemoveAt(CurveIndex);

			// delete this index from all poses
			for (int32 PoseIndex = 0; PoseIndex < Poses.Num(); ++PoseIndex)
			{
				Poses[PoseIndex].CurveData.RemoveAt(CurveIndex);
			}

			return true;
		}
	}

	return false;
}
void FPoseDataContainer::ConvertToFullPose(int32 InBasePoseIndex, const TArray<FTransform>& InBasePose, const TArray<float>& InBaseCurve)
{
	check(InBaseCurve.Num() == Curves.Num());

	for (int32 PoseIndex = 0; PoseIndex < Poses.Num(); ++PoseIndex)
	{
		// if this pose is not base pose
		if (PoseIndex != InBasePoseIndex)
		{
			// should it be move? Why? I need that buffer still
			TArray<FTransform> AdditivePose = Poses[PoseIndex].LocalSpacePose;
			FPoseData& PoseData = Poses[PoseIndex];

			check(AdditivePose.Num() == InBasePose.Num());
			for (int32 BoneIndex = 0; BoneIndex < AdditivePose.Num(); ++BoneIndex)
			{
				PoseData.LocalSpacePose[BoneIndex] = InBasePose[BoneIndex];
				PoseData.LocalSpacePose[BoneIndex].Accumulate(AdditivePose[BoneIndex]);
				PoseData.LocalSpacePose[BoneIndex].NormalizeRotation();
			}

			int32 CurveNum = Curves.Num();
			checkSlow(CurveNum == PoseData.CurveData.Num());
			for (int32 CurveIndex = 0; CurveIndex < CurveNum; ++CurveIndex)
			{
				PoseData.CurveData[CurveIndex] = InBaseCurve[CurveIndex] + PoseData.CurveData[CurveIndex];
			}
		}
	}
}

void FPoseDataContainer::ConvertToAdditivePose(int32 InBasePoseIndex, const TArray<FTransform>& InBasePose, const TArray<float>& InBaseCurve)
{
	check(InBaseCurve.Num() == Curves.Num());
	for (int32 PoseIndex = 0; PoseIndex < Poses.Num(); ++PoseIndex)
	{
		// if this pose is not base pose
		if (PoseIndex != InBasePoseIndex)
		{
			FPoseData& PoseData = Poses[PoseIndex];
			check(PoseData.LocalSpacePose.Num() == InBasePose.Num());
			for (int32 BoneIndex = 0; BoneIndex < InBasePose.Num(); ++BoneIndex)
			{
				FAnimationRuntime::ConvertTransformToAdditive(PoseData.LocalSpacePose[BoneIndex], InBasePose[BoneIndex]);
				PoseData.LocalSpacePose[BoneIndex].NormalizeRotation();
			}

			int32 CurveNum = Curves.Num();
			checkSlow(CurveNum == PoseData.CurveData.Num());
			for (int32 CurveIndex = 0; CurveIndex < CurveNum; ++CurveIndex)
			{
				PoseData.CurveData[CurveIndex] = PoseData.CurveData[CurveIndex] - InBaseCurve[CurveIndex];
			}
		}
	}
}

/////////////////////////////////////////////////////
// UPoseAsset
/////////////////////////////////////////////////////
UPoseAsset::UPoseAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, NumPoses(0)
	, bNormalizeWeight(true)
	, bAdditivePose(false)
	, BasePoseIndex(-1)
{
}

void UPoseAsset::GetBaseAnimationPose(struct FCompactPose& OutPose, FBlendedCurve& InOutCurve, const FAnimExtractContext& ExtractionContext) const
{
	OutPose.ResetToRefPose();

	if (bAdditivePose && PoseContainer.Poses.IsValidIndex(BasePoseIndex))
	{
		const FBoneContainer& RequiredBones = OutPose.GetBoneContainer();
		USkeleton* MySkeleton = GetSkeleton();

		// this contains compact bone pose list that this pose cares
		TArray<FCompactPoseBoneIndex> CompactBonePoseList;

		const int32 TrackNum = PoseContainer.TrackMap.Num();

		for (const TPair<FName, int32>& TrackPair : PoseContainer.TrackMap)
		{
			const int32 SkeletonBoneIndex = TrackPair.Value;
			const FCompactPoseBoneIndex PoseBoneIndex = RequiredBones.GetCompactPoseIndexFromSkeletonIndex(SkeletonBoneIndex);
			// we add even if it's invalid because we want it to match with track index
			CompactBonePoseList.Add(PoseBoneIndex);
		}

		const TArray<FTransform>& PoseTransform = PoseContainer.Poses[BasePoseIndex].LocalSpacePose;

		for (int32 TrackIndex = 0; TrackIndex < TrackNum; ++TrackIndex)
		{
			if (CompactBonePoseList[TrackIndex] != INDEX_NONE)
			{
				OutPose[CompactBonePoseList[TrackIndex]] = PoseTransform[TrackIndex];
			}
		}

		PoseContainer.GetPoseCurve(&PoseContainer.Poses[BasePoseIndex], InOutCurve);
	}
}

void UPoseAsset::GetAnimationPose(struct FCompactPose& OutPose, FBlendedCurve& OutCurve, const FAnimExtractContext& ExtractionContext) const
{
	const FBoneContainer& RequiredBones = OutPose.GetBoneContainer();
	USkeleton* MySkeleton = GetSkeleton();

	// this contains compact bone pose list that this pose cares
	TArray<FCompactPoseBoneIndex> CompactBonePoseList;

	const int32 TrackNum = PoseContainer.TrackMap.Num();

	for (const TPair<FName, int32>& TrackPair : PoseContainer.TrackMap)
	{
		const int32 SkeletonBoneIndex = TrackPair.Value;
		const FCompactPoseBoneIndex PoseBoneIndex = RequiredBones.GetCompactPoseIndexFromSkeletonIndex(SkeletonBoneIndex);
		// we add even if it's invalid because we want it to match with track index
		CompactBonePoseList.Add(PoseBoneIndex);
	}

	TMap<const FPoseData*, float> IndexToWeightMap;

	float TotalWeight = 0.f;

	check(PoseContainer.IsValid());

	for (int32 PoseIndex = 0; PoseIndex < PoseContainer.Poses.Num(); ++PoseIndex)
	{
		const FSmartName& PoseName = PoseContainer.PoseNames[PoseIndex];
		const FPoseData& PoseData = PoseContainer.Poses[PoseIndex];

		float Value = OutCurve.Get(PoseName.UID);

		// we only add to the list if it's not additive Or if it's additive, we don't want to add base pose index
		// and has weight
		if ((!bAdditivePose || PoseIndex != BasePoseIndex) && FAnimationRuntime::HasWeight(Value))
		{
			IndexToWeightMap.Add(&PoseData, Value);
			TotalWeight += Value;
		}
	}

	// need a copy of pose since we don't know if this can be additive
	FCompactPose CurrentPose;
	FBlendedCurve CurrentCurve;
	CurrentPose.SetBoneContainer(&OutPose.GetBoneContainer());
	CurrentCurve.InitFrom(OutCurve);

	if (bAdditivePose)
	{
		CurrentPose.ResetToIdentity();
	}
	else
	{
		CurrentPose.ResetToRefPose();
	}

	const int32 TotalNumberOfValidPoses = IndexToWeightMap.Num();
	if (TotalNumberOfValidPoses > 0)
	{
		TArray<FTransform> BlendedBoneTransform;
		bool bFirstPose = true;
		TArray<FBlendedCurve> PoseCurves;
		TArray<float>	CurveWeights;

		//if full pose, we'll have to normalize by weight
		if (bNormalizeWeight && TotalWeight != 1.f)
		{
			for (TPair<const FPoseData*, float>& WeightPair : IndexToWeightMap)
			{
				WeightPair.Value /= TotalWeight;
			}
		}

		PoseCurves.AddDefaulted(TotalNumberOfValidPoses);
		CurveWeights.AddUninitialized(TotalNumberOfValidPoses);
		int32 PoseIndex = 0;
		for (const TPair<const FPoseData*, float>& ActivePosePair : IndexToWeightMap)
		{
			const FPoseData* Pose = ActivePosePair.Key;
			const float Weight = ActivePosePair.Value;

			if (bFirstPose)
			{
				BlendedBoneTransform.AddUninitialized(Pose->LocalSpacePose.Num());
				for (int32 TrackIndex = 0; TrackIndex < Pose->LocalSpacePose.Num(); ++TrackIndex)
				{
					BlendTransform<ETransformBlendMode::Overwrite>(Pose->LocalSpacePose[TrackIndex], BlendedBoneTransform[TrackIndex], Weight);
				}
				bFirstPose = false;
			}
			else
			{
				for (int32 TrackIndex = 0; TrackIndex < Pose->LocalSpacePose.Num(); ++TrackIndex)
				{
					BlendTransform<ETransformBlendMode::Accumulate>(Pose->LocalSpacePose[TrackIndex], BlendedBoneTransform[TrackIndex], Weight);
				}
			}

			CurveWeights[PoseIndex] = Weight;
			PoseCurves[PoseIndex].InitFrom(OutCurve);
			PoseContainer.GetPoseCurve(Pose, PoseCurves[PoseIndex]);
			++PoseIndex;
		}

		// blend curves
		BlendCurves(PoseCurves, CurveWeights, CurrentCurve);

		for (int32 TrackIndex = 0; TrackIndex < TrackNum; ++TrackIndex)
		{
			if (CompactBonePoseList[TrackIndex] != INDEX_NONE)
			{
				CurrentPose[CompactBonePoseList[TrackIndex]] = BlendedBoneTransform[TrackIndex];
				CurrentPose.NormalizeRotations();
			}
		}

		if (bAdditivePose)
		{
			FAnimationRuntime::AccumulateAdditivePose(OutPose, CurrentPose, OutCurve, CurrentCurve, TotalWeight, AAT_LocalSpaceBase);
			OutPose.NormalizeRotations();
		}
		else
		{
			OutPose = CurrentPose;
			OutCurve = CurrentCurve;
		}
	}
}

void UPoseAsset::PostLoad()
{
	Super::PostLoad();

	// fix curve names
	USkeleton* MySkeleton = GetSkeleton();
	if (MySkeleton)
	{
		MySkeleton->VerifySmartNames(USkeleton::AnimCurveMappingName, PoseContainer.PoseNames);

		for (auto& Curve : PoseContainer.Curves)
		{
			MySkeleton->VerifySmartName(USkeleton::AnimCurveMappingName, Curve.Name);
		}
	}

	// I  have to fix pose names
	RecacheTrackmap();
}

void UPoseAsset::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);
}

const TArray<FSmartName> UPoseAsset::GetPoseNames() const
{
	return PoseContainer.PoseNames;
}

const TArray<FSmartName> UPoseAsset::GetCurveNames() const
{
	TArray<FSmartName> CurveNames;
	for (int32 CurveIndex = 0; CurveIndex < PoseContainer.Curves.Num(); ++CurveIndex)
	{
		CurveNames.Add(PoseContainer.Curves[CurveIndex].Name);
	}

	return CurveNames;
}


bool UPoseAsset::ContainsPose(const FName& InPoseName) const
{
	for (const auto& PoseName : PoseContainer.PoseNames)
	{
		if (PoseName.DisplayName == InPoseName)
		{
			return true;
		}
	}

	return false;
}

void UPoseAsset::AddOrUpdatePose(const FSmartName& PoseName, USkeletalMeshComponent* MeshComponent)
{
	USkeleton* MySkeleton = GetSkeleton();
	if (MySkeleton && MeshComponent && MeshComponent->SkeletalMesh)
	{
		TArray<FName> TrackNames;
		// note this ignores root motion
		TArray<FTransform> BoneTransform = MeshComponent->GetSpaceBases();
		const FReferenceSkeleton& RefSkeleton = MeshComponent->SkeletalMesh->RefSkeleton;
		for (int32 BoneIndex = 0; BoneIndex < RefSkeleton.GetNum(); ++BoneIndex)
		{
			TrackNames.Add(RefSkeleton.GetBoneName(BoneIndex));
		}

		// convert to local space
		for (int32 BoneIndex = BoneTransform.Num() - 1; BoneIndex >= 0; --BoneIndex)
		{
			const int32 ParentIndex = RefSkeleton.GetParentIndex(BoneIndex);
			if (ParentIndex != INDEX_NONE)
			{
				BoneTransform[BoneIndex] = BoneTransform[BoneIndex].GetRelativeTransform(BoneTransform[ParentIndex]);
			}
		}

		AddOrUpdatePose(PoseName, TrackNames, BoneTransform);
	}
}

void UPoseAsset::AddOrUpdatePose(const FSmartName& PoseName, TArray<FName> TrackNames, TArray<FTransform>& LocalTransform)
{
	USkeleton* MySkeleton = GetSkeleton();
	if (MySkeleton)
	{
		// first combine track, we want to make sure all poses contains tracks with this
		CombineTracks(TrackNames);

		FPoseData* PoseData = PoseContainer.FindOrAddPoseData(PoseName);
		// now copy all transform back to it. 
		check(PoseData);
		PoseContainer.FillUpDefaultPose(PoseData, MySkeleton, RetargetSource);

		const FReferenceSkeleton& RefSkeleton = MySkeleton->GetReferenceSkeleton();
		for (int32 Index = 0; Index < TrackNames.Num(); ++Index)
		{
			// now get poseData track index
			const FName& TrackName = TrackNames[Index];
			int32 SkeletonIndex = RefSkeleton.FindBoneIndex(TrackName);
			int32 InternalTrackIndex = PoseContainer.Tracks.Find(TrackName);
			PoseData->LocalSpacePose[InternalTrackIndex] = LocalTransform[Index];
		}

		NumPoses = PoseContainer.GetNumPoses();
	}
}

void UPoseAsset::CombineTracks(const TArray<FName>& NewTracks)
{
	USkeleton* MySkeleton = GetSkeleton();
	if (MySkeleton)
	{
		for (const auto& NewTrack : NewTracks)
		{
			if (PoseContainer.Tracks.Contains(NewTrack) == false)
			{
				// if we don't have it, then we'll have to add this track and then 
				// right now it doesn't have to be in the hierarchy
				// @todo: it is probably best to keep the hierarchy of the skeleton, so in the future, we might like to sort this by track after
				PoseContainer.InsertTrack(NewTrack, MySkeleton, RetargetSource);
			}
		}
	}
}

FName GetUniquePoseName(USkeleton* Skeleton)
{
	check(Skeleton);
	int32 NameIndex = 0;

	FSmartNameMapping::UID NewUID;
	FName NewName;

	do
	{
		NewName = FName(*FString::Printf(TEXT("Pose_%d"), NameIndex++));
		NewUID = Skeleton->GetUIDByName(USkeleton::AnimCurveMappingName, NewName);
	} while (NewUID != FSmartNameMapping::MaxUID);

	// if found, 

	return NewName;
}

#if WITH_EDITOR
void UPoseAsset::CreatePoseFromAnimation(class UAnimSequence* AnimSequence)
{
	if (AnimSequence)
	{
		USkeleton* TargetSkeleton = AnimSequence->GetSkeleton();
		if (TargetSkeleton)
		{
			SetSkeleton(TargetSkeleton);

			PoseContainer.Reset();

			NumPoses = AnimSequence->GetNumberOfFrames();

			// make sure we have more than one pose
			if (NumPoses > 0)
			{
				// stack allocator for extracting curve
				FMemMark Mark(FMemStack::Get());

				const int32 NumTracks = AnimSequence->AnimationTrackNames.Num();

				// set up track data - @todo: add revaliation code when checked
				for (int32 TrackIndex = 0; TrackIndex < NumTracks; ++TrackIndex)
				{
					const FName& TrackName = AnimSequence->AnimationTrackNames[TrackIndex];
					PoseContainer.Tracks.Add(TrackName);
				}

				// now create pose transform
				TArray<FTransform> NewPose;
				
				NewPose.Reset(NumTracks);
				NewPose.AddUninitialized(NumTracks);

				// @Todo fill up curve data
				TArray<float> CurveData;
				float IntervalBetweenKeys = AnimSequence->SequenceLength / NumPoses;

				// add curves - only float curves
				const int32 TotalFloatCurveCount = AnimSequence->RawCurveData.FloatCurves.Num();
				
				// have to construct own UIDList;
				TArray<FSmartNameMapping::UID> UIDList;

				if (TotalFloatCurveCount > 0)
				{
					for (const FFloatCurve& Curve : AnimSequence->RawCurveData.FloatCurves)
					{
						PoseContainer.Curves.Add(FAnimCurveBase(Curve.Name, Curve.GetCurveTypeFlags()));
						UIDList.Add(Curve.Name.UID);
					}
				}

				CurveData.AddZeroed(UIDList.Num());
				// add to skeleton UID, so that it knows the curve data
				for (int32 PoseIndex = 0; PoseIndex < NumPoses; ++PoseIndex)
				{
					FName PoseName = GetUniquePoseName(TargetSkeleton);
					// now get rawanimationdata, and each key is converted to new pose
					for (int32 TrackIndex = 0; TrackIndex < NumTracks; ++TrackIndex)
					{
						const auto& RawTrack = AnimSequence->RawAnimationData[TrackIndex];
						AnimSequence->ExtractBoneTransform(RawTrack, NewPose[TrackIndex], PoseIndex);
					}

					// add to skeleton UID, so that it knows the curve data
					FSmartName NewPoseName;
					TargetSkeleton->AddSmartNameAndModify(USkeleton::AnimCurveMappingName, PoseName, NewPoseName);

					if (TotalFloatCurveCount > 0)
					{
						// get curve data
						// have to do iterate over time
						// support curve
						FBlendedCurve SourceCurve;
						SourceCurve.InitFrom(&UIDList);
						AnimSequence->EvaluateCurveData(SourceCurve, PoseIndex*IntervalBetweenKeys, true);

						// copy back to CurveData
						for (int32 CurveIndex = 0; CurveIndex < CurveData.Num(); ++CurveIndex)
						{
							CurveData[CurveIndex] = SourceCurve.Get(UIDList[CurveIndex]);
						}

						check(CurveData.Num() == PoseContainer.Curves.Num());
					}
				
					// add new pose
					PoseContainer.AddOrUpdatePose(NewPoseName, NewPose, CurveData);
				}


				PoseContainer.Shrink(TargetSkeleton, RetargetSource);
				RecacheTrackmap();
			}
		}
	}
}

#endif // WITH_EDITOR

bool UPoseAsset::ModifyPoseName(FName OldPoseName, FName NewPoseName, const FSmartNameMapping::UID* NewUID)
{
	USkeleton* MySkeleton = GetSkeleton();

	if (ContainsPose(NewPoseName))
	{
		// already exists, return 
		return false;
	}

	FSmartName OldPoseSmartName;
	ensureAlways(MySkeleton->GetSmartNameByName(USkeleton::AnimCurveMappingName, OldPoseName, OldPoseSmartName));

	FPoseData* PoseData = PoseContainer.FindPoseData(OldPoseSmartName);
	if (PoseData)
	{
		FSmartName NewPoseSmartName;
		if (NewUID)
		{
			MySkeleton->GetSmartNameByUID(USkeleton::AnimCurveMappingName, *NewUID, NewPoseSmartName);
		}
		else
		{
			// we're renaming current one
			MySkeleton->RenameSmartName(USkeleton::AnimCurveMappingName, OldPoseSmartName.UID, NewPoseName);
			NewPoseSmartName = OldPoseSmartName;
			NewPoseSmartName.DisplayName = NewPoseName;
		}

		PoseContainer.RenamePose(OldPoseSmartName, NewPoseSmartName);
		OnPoseListChanged.Broadcast();

		return true;
	}

	return false;
}

int32 UPoseAsset::DeletePoses(TArray<FName> PoseNamesToDelete)
{
	int32 ItemsDeleted = 0;

	USkeleton* MySkeleton = GetSkeleton();

	for (const auto& PoseName : PoseNamesToDelete)
	{
		FSmartName PoseSmartName;
		if (MySkeleton->GetSmartNameByName(USkeleton::AnimCurveMappingName, PoseName, PoseSmartName) 
			&& 	PoseContainer.DeletePose(PoseSmartName))
		{
			++ItemsDeleted;
		}
	}

	PoseContainer.Shrink(GetSkeleton(), RetargetSource);

	OnPoseListChanged.Broadcast();

	return ItemsDeleted;
}

int32 UPoseAsset::DeleteCurves(TArray<FName> CurveNamesToDelete)
{
	int32 ItemsDeleted = 0;

	USkeleton* MySkeleton = GetSkeleton();

	for (const auto& CurveName : CurveNamesToDelete)
	{
		FSmartName CurveSmartName;
		if (MySkeleton->GetSmartNameByName(USkeleton::AnimCurveMappingName, CurveName, CurveSmartName))
		{
			PoseContainer.DeleteCurve(CurveSmartName);
			++ItemsDeleted;
		}
	}

	OnPoseListChanged.Broadcast();

	return ItemsDeleted;
}
bool UPoseAsset::ConvertToFullPose()
{
	if (ensureAlways(bAdditivePose))
	{
		TArray<FTransform>	BasePose;
		TArray<float>		BaseCurves;
		GetBasePoseTransform(BasePose, BaseCurves);

		PoseContainer.ConvertToFullPose(BasePoseIndex, BasePose, BaseCurves);

		bAdditivePose = false;

		return true;
	}

	return false;
}

bool UPoseAsset::ConvertToAdditivePose(int32 NewBasePoseIndex)
{
	if (ensureAlways(!bAdditivePose))
	{
		// make sure it's valid
		check(NewBasePoseIndex == -1 || PoseContainer.Poses.IsValidIndex(NewBasePoseIndex));

		BasePoseIndex = NewBasePoseIndex;

		TArray<FTransform> BasePose;
		TArray<float>		BaseCurves;
		GetBasePoseTransform(BasePose, BaseCurves);

		PoseContainer.ConvertToAdditivePose(BasePoseIndex, BasePose, BaseCurves);

		bAdditivePose = true;

		return true;
	}

	return false;
}

bool UPoseAsset::ConvertSpace(bool bNewAdditivePose, int32 NewBasePoseInde)
{
	// first convert to full pose first
	if (bAdditivePose)
	{
		// convert to full pose
		if (!ConvertToFullPose())
		{
			// issue with converting full pose
			return false;
		}
	}

	// now we have full pose
	if (bNewAdditivePose)
	{
		ConvertToAdditivePose(NewBasePoseInde);
	}

	return true;
}

const int32 UPoseAsset::GetPoseIndexByName(const FName& InBasePoseName) const
{
	for (int32 PoseIndex = 0; PoseIndex < PoseContainer.PoseNames.Num(); ++PoseIndex)
	{
		if (PoseContainer.PoseNames[PoseIndex].DisplayName == InBasePoseName)
		{
			return PoseIndex;
		}
	}

	return INDEX_NONE;
}

void UPoseAsset::RecacheTrackmap()
{
	USkeleton* MySkeleton = GetSkeleton();
	PoseContainer.TrackMap.Reset();

	if (MySkeleton)
	{
		const FReferenceSkeleton& RefSkeleton = MySkeleton->GetReferenceSkeleton();

		// set up track data - @todo: add revaliation code when checked
		for (int32 TrackIndex = 0; TrackIndex < PoseContainer.Tracks.Num(); ++TrackIndex)
		{
			const FName& TrackName = PoseContainer.Tracks[TrackIndex];
			const int32 SkeletonTrackIndex = RefSkeleton.FindBoneIndex(TrackName);
			PoseContainer.TrackMap.Add(TrackName, SkeletonTrackIndex);
		}
	}
}

#if WITH_EDITOR
void UPoseAsset::RemapTracksToNewSkeleton(USkeleton* NewSkeleton, bool bConvertSpaces)
{
	Super::RemapTracksToNewSkeleton(NewSkeleton, bConvertSpaces);
	// @todo: should allow deleted tracks and so on
	RecacheTrackmap();
}
#endif // WITH_EDITOR

bool UPoseAsset::GetBasePoseTransform(TArray<FTransform>& OutBasePose, TArray<float>& OutCurve) const
{
	int32 TotalNumTrack = PoseContainer.Tracks.Num();
	OutBasePose.Reset(TotalNumTrack);

	if (BasePoseIndex == -1)
	{
		OutBasePose.AddUninitialized(TotalNumTrack);
		for (int32 TrackIndex = 0; TrackIndex < PoseContainer.Tracks.Num(); ++TrackIndex)
		{
			const FName& TrackName = PoseContainer.Tracks[TrackIndex];
			OutBasePose[TrackIndex] = PoseContainer.GetDefaultTransform(TrackName, GetSkeleton(), RetargetSource);
		}

		// add zero curves
		OutCurve.AddZeroed(PoseContainer.Curves.Num());
		check(OutBasePose.Num() == TotalNumTrack);
		return true;
	}
	else if (PoseContainer.Poses.IsValidIndex(BasePoseIndex))
	{
		OutBasePose = PoseContainer.Poses[BasePoseIndex].LocalSpacePose;
		OutCurve = PoseContainer.Poses[BasePoseIndex].CurveData;
		check(OutBasePose.Num() == TotalNumTrack);
		return true;
	}

	return false;
}