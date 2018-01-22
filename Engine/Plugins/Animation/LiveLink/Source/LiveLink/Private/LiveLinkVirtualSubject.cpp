// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "LiveLinkVirtualSubject.h"

bool FLiveLinkVirtualSubject::DoesRefSkeletonNeedRebuilding(const TMap<FName, FLiveLinkSubjectFrame>& ActiveSubjectSnapshots) const
{
	check(Subjects.Num() >= 1);

	if (Subjects.Num() != SubjectRefSkeletonGuids.Num()) // If the arrays don't match we need a new skeleton
	{
		return true;
	}

	for (int32 Index = 0; Index < Subjects.Num(); ++Index)
	{
		const FLiveLinkSubjectFrame& SubjectFrame = ActiveSubjectSnapshots.FindChecked(Subjects[Index]);
		if (SubjectFrame.RefSkeletonGuid != SubjectRefSkeletonGuids[Index])
		{
			return true;
		}
	}
	return false;
}

void AddToBoneNames(TArray<FName>& BoneNames, const TArray<FName>& NewBoneNames, const FName Prefix)
{
	FString NameFormat = Prefix.ToString() + TEXT("_");

	BoneNames.Reserve(BoneNames.Num() + NewBoneNames.Num());

	for (const FName& NewBoneName : NewBoneNames)
	{
		BoneNames.Add(*(NameFormat + NewBoneName.ToString()));
	}
}

void AddToBoneParents(TArray<int32>& BoneParents, const TArray<int32>& NewBoneParents)
{
	const int32 Offset = BoneParents.Num();

	BoneParents.Reserve(BoneParents.Num() + NewBoneParents.Num());

	for (int32 BoneParent : NewBoneParents)
	{
		// Here we are combining multiple bone hierarchies under one root bone
		// Each hierarchy is complete self contained so we have a simple calculation to perform
		// 1) Bones with out a parent get parented to root (-1 => 0 )
		// 2) Bones with a parent need and offset based on the current size of the buffer
		if (BoneParent == INDEX_NONE)
		{
			BoneParents.Add(0);
		}
		else
		{
			BoneParents.Add(BoneParent + Offset);
		}
	}
}

void FLiveLinkVirtualSubject::BuildRefSkeletonForVirtualSubject(const TMap<FName, FLiveLinkSubjectFrame>& ActiveSubjectSnapshots, const TArray<FName>& ActiveSubjectNames)
{
	ValidateCurrentSubjects(ActiveSubjectNames);

	if (DoesRefSkeletonNeedRebuilding(ActiveSubjectSnapshots))
	{
		TArray<FName> BoneNames{ TEXT("Root") };
		TArray<int32> BoneParents{ INDEX_NONE };

		for (FName SubjectName : Subjects)
		{
			const FLiveLinkSubjectFrame& SubjectFrame = ActiveSubjectSnapshots.FindChecked(SubjectName);

			AddToBoneNames(BoneNames, SubjectFrame.RefSkeleton.GetBoneNames(), SubjectName);
			AddToBoneParents(BoneParents, SubjectFrame.RefSkeleton.GetBoneParents());
		}

		RefSkeleton.SetBoneNames(BoneNames);
		RefSkeleton.SetBoneParents(BoneParents);
	}
}