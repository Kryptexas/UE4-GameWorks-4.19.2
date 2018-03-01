// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "LiveLinkRemapAsset.h"
#include "LiveLinkTypes.h"
#include "BonePose.h"
#include "Engine/Blueprint.h"
  
ULiveLinkRemapAsset::ULiveLinkRemapAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	UBlueprint* Blueprint = Cast<UBlueprint>(GetClass()->ClassGeneratedBy);
	if (Blueprint)
	{
		OnBlueprintCompiledDelegate = Blueprint->OnCompiled().AddUObject(this, &ULiveLinkRemapAsset::OnBlueprintClassCompiled);
	}
}

void ULiveLinkRemapAsset::BeginDestroy()
{
	if (OnBlueprintCompiledDelegate.IsValid())
	{
		UBlueprint* Blueprint = Cast<UBlueprint>(GetClass()->ClassGeneratedBy);
		check(Blueprint);
		Blueprint->OnCompiled().Remove(OnBlueprintCompiledDelegate);
		OnBlueprintCompiledDelegate.Reset();
	}

	Super::BeginDestroy();
}

void ULiveLinkRemapAsset::OnBlueprintClassCompiled(UBlueprint* TargetBlueprint)
{
	BoneNameMap.Reset();
	CurveNameMap.Reset();
}

void MakeCurveMapFromFrame(const FCompactPose& InPose, const FLiveLinkSubjectFrame& InFrame, const TArray<FName, TMemStackAllocator<>>& TransformedCurveNames, TMap<FName, float>& OutCurveMap)
{
	OutCurveMap.Reset();
	OutCurveMap.Reserve(InFrame.Curves.Num());

	const USkeleton* Skeleton = InPose.GetBoneContainer().GetSkeletonAsset();

	for (int32 CurveIdx = 0; CurveIdx < InFrame.CurveKeyData.CurveNames.Num(); ++CurveIdx)
	{
		const FOptionalCurveElement& Curve = InFrame.Curves[CurveIdx];
		if (Curve.IsValid())
		{
			OutCurveMap.Add(TransformedCurveNames[CurveIdx]) = Curve.Value;
		}
	}
}


void ULiveLinkRemapAsset::BuildPoseForSubject(float DeltaTime, const FLiveLinkSubjectFrame& InFrame, FCompactPose& OutPose, FBlendedCurve& OutCurve)
{
	const TArray<FName>& SourceBoneNames = InFrame.RefSkeleton.GetBoneNames();

	TArray<FName, TMemStackAllocator<>> TransformedBoneNames;
	TransformedBoneNames.Reserve(SourceBoneNames.Num());

	for (const FName& SrcBoneName : SourceBoneNames)
	{
		FName* TargetBoneName = BoneNameMap.Find(SrcBoneName);
		if (TargetBoneName == nullptr)
		{
			FName NewName = GetRemappedBoneName(SrcBoneName);
			TransformedBoneNames.Add(NewName);
			BoneNameMap.Add(SrcBoneName, NewName);
		}
		else
		{
			TransformedBoneNames.Add(*TargetBoneName);
		}
	}

	for (int32 i = 0; i < TransformedBoneNames.Num(); ++i)
	{
		FName BoneName = TransformedBoneNames[i];

		FTransform BoneTransform = InFrame.Transforms[i];

		int32 MeshIndex = OutPose.GetBoneContainer().GetPoseBoneIndexForBoneName(BoneName);
		if (MeshIndex != INDEX_NONE)
		{
			FCompactPoseBoneIndex CPIndex = OutPose.GetBoneContainer().MakeCompactPoseIndex(FMeshPoseBoneIndex(MeshIndex));
			if (CPIndex != INDEX_NONE)
			{
				OutPose[CPIndex] = BoneTransform;
			}
		}
	}

	const TArray<FName>& SourceCurveNames = InFrame.CurveKeyData.CurveNames;
	TArray<FName, TMemStackAllocator<>> TransformedCurveNames;
	TransformedCurveNames.Reserve(SourceCurveNames.Num());

	for(const FName& SrcCurveName : SourceCurveNames)
	{
		FName* TargetCurveName = CurveNameMap.Find(SrcCurveName);
		if (TargetCurveName == nullptr)
		{
			FName NewName = GetRemappedCurveName(SrcCurveName);
			TransformedCurveNames.Add(NewName);
			CurveNameMap.Add(SrcCurveName, NewName);
		}
		else
		{
			TransformedCurveNames.Add(*TargetCurveName);
		}
	}

	TMap<FName, float> BPCurveValues;

	MakeCurveMapFromFrame(OutPose, InFrame, TransformedCurveNames, BPCurveValues);

	RemapCurveElements(BPCurveValues);

	BuildCurveData(BPCurveValues, OutPose, OutCurve);
}

FName ULiveLinkRemapAsset::GetRemappedBoneName_Implementation(FName BoneName) const
{
	return BoneName;
}

FName ULiveLinkRemapAsset::GetRemappedCurveName_Implementation(FName CurveName) const
{
	return CurveName;
}

void ULiveLinkRemapAsset::RemapCurveElements_Implementation(TMap<FName, float>& CurveItems) const
{
}