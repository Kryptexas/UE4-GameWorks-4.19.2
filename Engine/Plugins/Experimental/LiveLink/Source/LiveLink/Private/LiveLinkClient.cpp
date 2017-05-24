// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "LiveLinkClient.h"
#include "ScopeLock.h"
#include "UObjectHash.h"
#include "LiveLinkSourceFactory.h"
#include "Guid.h"

const TArray<FTransform>& FLiveLinkTransformData::GetTransformData()
{
	return Transforms.Last();
}

FLiveLinkClient::~FLiveLinkClient()
{
	TArray<int> ToRemove;
	ToRemove.Reserve(Sources.Num());

	while (Sources.Num() > 0)
	{
		ToRemove.Reset();

		for (int32 Idx = 0; Idx < Sources.Num(); ++Idx)
		{
			if (Sources[Idx]->RequestSourceShutdown())
			{
				ToRemove.Add(Idx);
			}
		}

		for (int32 Idx = ToRemove.Num() - 1; Idx >= 0; --Idx)
		{
			delete Sources[Idx];
			Sources.RemoveAtSwap(ToRemove[Idx],1,false);
		}
	}
}

void FLiveLinkClient::AddSource(ILiveLinkSource* InSource)
{
	Sources.Add(InSource);
	SourceGuids.Add(FGuid::NewGuid());

	InSource->ReceiveClient(this);
}

void FLiveLinkClient::PushSubjectSkeleton(FName SubjectName, const FLiveLinkRefSkeleton& RefSkeleton)
{
	FScopeLock Lock(&SubjectDataAccessCriticalSection);
	
	if (FSubjectData* SubjectData = Subjects.Find(SubjectName))
	{
		//LIVELINK TODO
		//Handle subject hierarchy changing
	}
	else
	{
		Subjects.Emplace(SubjectName, FSubjectData(RefSkeleton));
	}
}

void FLiveLinkClient::PushSubjectData(FName SubjectName, const TArray<FTransform>& Transforms)
{
	FScopeLock Lock(&SubjectDataAccessCriticalSection);

	if (FSubjectData* SubjectData = Subjects.Find(SubjectName))
	{
		SubjectData->TransformData.PushTransformData(Transforms);
	}
}

bool FLiveLinkClient::GetSubjectData(FName SubjectName, TArray<FTransform>& OutTransforms, FLiveLinkRefSkeleton& OutRefSkeleton)
{
	FScopeLock Lock(&SubjectDataAccessCriticalSection);

	if (FSubjectData* SubjectData = Subjects.Find(SubjectName))
	{
		if (SubjectData->TransformData.HasTransformData())
		{
			OutTransforms = SubjectData->TransformData.GetTransformData();
			OutRefSkeleton = SubjectData->RefSkeleton;
			return true;
		}
	}
	return false;
}

ILiveLinkSource* FLiveLinkClient::GetSourceForGUID(FGuid InEntryGuid) const
{
	int32 Idx = SourceGuids.IndexOfByKey(InEntryGuid);
	return Idx != INDEX_NONE ? Sources[Idx] : nullptr;
}

FText FLiveLinkClient::GetSourceTypeForEntry(FGuid InEntryGuid) const
{
	if (ILiveLinkSource* Source = GetSourceForGUID(InEntryGuid))
	{
		return Source->GetSourceType();
	}
	return FText(NSLOCTEXT("TempLocTextLiveLink","InvalidSourceType", "Invalid Source Type"));
}

FText FLiveLinkClient::GetMachineNameForEntry(FGuid InEntryGuid) const
{
	if (ILiveLinkSource* Source = GetSourceForGUID(InEntryGuid))
	{
		return Source->GetSourceMachineName();
	}
	return FText(NSLOCTEXT("TempLocTextLiveLink","InvalidSourceMachineName", "Invalid Source Machine Name"));
}

FText FLiveLinkClient::GetEntryStatusForEntry(FGuid InEntryGuid) const
{
	if (ILiveLinkSource* Source = GetSourceForGUID(InEntryGuid))
	{
		return Source->GetSourceStatus();
	}
	return FText(NSLOCTEXT("TempLocTextLiveLink","InvalidSourceStatus", "Invalid Source Status"));
}
