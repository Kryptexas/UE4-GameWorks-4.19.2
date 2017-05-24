// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Transform.h"
#include "CriticalSection.h"

#include "ILiveLinkClient.h"
#include "ILiveLinkSource.h"
#include "LiveLinkRefSkeleton.h"

struct FLiveLinkTransformData
{
public:

	const TArray<FTransform>& GetTransformData();

	bool HasTransformData() const { return Transforms.Num() > 0; }

	void PushTransformData(const TArray<FTransform>& InTransforms)
	{
		if (Transforms.Num() == 0)
		{
			Transforms.Add(InTransforms);
		}
		else
		{
			Transforms.Last() = InTransforms;
		}
	}
private:

	TArray<TArray<FTransform>> Transforms;
};

struct FSubjectData
{
public:
	FLiveLinkRefSkeleton RefSkeleton;
	
	FLiveLinkTransformData TransformData;

	FSubjectData(const FLiveLinkRefSkeleton& InRefSkeleton) 
		: RefSkeleton(InRefSkeleton)
	{}
};

class FLiveLinkClient : public ILiveLinkClient
{
public:
	FLiveLinkClient() {}
	~FLiveLinkClient();

	void AddSource(ILiveLinkSource* InSource);

	virtual void PushSubjectSkeleton(FName SubjectName, const FLiveLinkRefSkeleton& RefSkeleton);

	virtual void PushSubjectData(FName SubjectName, const TArray<FTransform>& Transforms);

	virtual bool GetSubjectData(FName SubjectName, TArray<FTransform>& OutTransforms, FLiveLinkRefSkeleton& OutRefSkeleton);

	const TArray<ILiveLinkSource*>& GetSources() const { return Sources; }
	const TArray<FGuid>& GetSourceEntries() const { return SourceGuids; }

	FText GetSourceTypeForEntry(FGuid InEntryGuid) const;
	FText GetMachineNameForEntry(FGuid InEntryGuid) const;
	FText GetEntryStatusForEntry(FGuid InEntryGuid) const;
private:

	ILiveLinkSource* GetSourceForGUID(FGuid InEntryGuid) const;

	// Current streamed data for subjects
	TMap<FName, FSubjectData> Subjects;

	// Current Sources
	TArray<ILiveLinkSource*> Sources;
	TArray<FGuid>			 SourceGuids;


	FCriticalSection SubjectDataAccessCriticalSection;
};