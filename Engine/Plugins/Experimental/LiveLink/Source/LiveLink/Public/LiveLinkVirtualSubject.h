// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"

#include "LiveLinkRefSkeleton.h"
#include "LiveLinkTypes.h"

#include "LiveLinkVirtualSubject.generated.h"


// A Virtual subject is made up of one or more real subjects from a source
USTRUCT()
struct FLiveLinkVirtualSubject
{
public:
	GENERATED_BODY()

	// Names of the real subjects to combine into a virtual subject
	UPROPERTY(EditAnywhere, Category = "General")
	TArray<FName>	Subjects;

	// Key for storing curve data (Names)
	FLiveLinkCurveKey	 CurveKeyData;

	// Source that the real subjects come from
	UPROPERTY(EditAnywhere, Category = "General")
	FGuid			Source;

	// Invalidates cache subject data (will force ref skeleton rebuild)
	void InvalidateSubjectGuids()
	{
		SubjectRefSkeletonGuids.Reset();
	}

	// Validates current source subjects
	void ValidateCurrentSubjects(const TArray<FName>& ValidSubjectNames)
	{
		const int32 Removed = Subjects.RemoveAll([ValidSubjectNames](const FName& Name) { return !ValidSubjectNames.Contains(Name); });
		if (Removed > 0)
		{
			SubjectRefSkeletonGuids.Reset();
		}
	}

	// Get virtual subject data
	const FLiveLinkRefSkeleton& GetRefSkeleton() const { return RefSkeleton; }
	const TArray<FName>& GetSubjects() const { return Subjects; }
	const TArray<FGuid>& GetSubjectRefSkeletonGuids() const { return SubjectRefSkeletonGuids; }

	// Builds a new ref skeleton based on the current subject state (can early out if ref skeleton is already up to date)
	void BuildRefSkeletonForVirtualSubject(const TMap<FName, FLiveLinkSubjectFrame>& ActiveSubjectSnapshots, const TArray<FName>& ActiveSubjectNames);

private:
	// Tests to see if current ref skeleton is up to data
	bool DoesRefSkeletonNeedRebuilding(const TMap<FName, FLiveLinkSubjectFrame>& ActiveSubjectSnapshots) const;

	// Ref Skeleton for the virtual subject
	FLiveLinkRefSkeleton RefSkeleton;

	// Guids for the real subjects ref skeletons (so we can track when we need to update our own ref skeleton
	TArray<FGuid> SubjectRefSkeletonGuids;
};