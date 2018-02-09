// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Transform.h"
#include "CriticalSection.h"
#include "Tickable.h"
#include "GCObject.h"

#include "ILiveLinkClient.h"
#include "ILiveLinkSource.h"
#include "LiveLinkRefSkeleton.h"
#include "LiveLinkTypes.h"
#include "LiveLinkSourceSettings.h"
#include "LiveLinkVirtualSubject.h"

// Live Link Log Category
DECLARE_LOG_CATEGORY_EXTERN(LogLiveLink, Log, All);

struct FLiveLinkFrame
{
public:
	TArray<FTransform>		Transforms;
	TArray<FOptionalCurveElement>	Curves;

	FLiveLinkMetaData		MetaData;

	FLiveLinkWorldTime WorldTime;

	void ExtendCurveData(int32 ExtraCurves)
	{
		Curves.AddDefaulted(ExtraCurves);
	}
};

struct FLiveLinkSubject
{
	// Key for storing curve data (Names)
	FLiveLinkCurveKey	 CurveKeyData;

	// Subject data frames that we have received (transforms and curve values)
	TArray<FLiveLinkFrame>		Frames;

	// Time difference between current system time and TimeCode times 
	double SubjectTimeOffset;

	// Last time we read a frame from this subject. Used to determine whether any new incoming
	// frames are useable
	double LastReadTime;

	//Cache of the last frame we read from, Used for frame cleanup
	int32 LastReadFrame;

	// Guid to track the last live link source that modified us
	FGuid LastModifier;

	// Connection settings specified by user
	FLiveLinkInterpolationSettings CachedInterpolationSettings;

	FLiveLinkSubject(const FLiveLinkRefSkeleton& InRefSkeleton)
		: RefSkeleton(InRefSkeleton)
		, RefSkeletonGuid(FGuid::NewGuid())
	{}

	FLiveLinkSubject()
	{}

	// Add a frame of data from a FLiveLinkFrameData
	void AddFrame(const FLiveLinkFrameData& FrameData, FGuid FrameSource);

	// Populate OutFrame with a frame based off of the supplied time and our own offsets
	void BuildInterpolatedFrame(const double InSeconds, FLiveLinkSubjectFrame& OutFrame);

	// Get this subjects ref skeleton
	const FLiveLinkRefSkeleton& GetRefSkeleton() const { return RefSkeleton; }

	// Handling setting a new ref skeleton
	void SetRefSkeleton(const FLiveLinkRefSkeleton& InRefSkeleton) { RefSkeleton = InRefSkeleton; RefSkeletonGuid = FGuid::NewGuid(); }

private:

	// Ref Skeleton for transforms
	FLiveLinkRefSkeleton RefSkeleton;

	// Allow us to track changes to the ref skeleton
	FGuid RefSkeletonGuid;
};

// Structure that identifies an individual subject
struct FLiveLinkSubjectKey
{
	// The Name of this subject
	FName SubjectName;

	// The guid for this subjects source
	FGuid Source;

	FLiveLinkSubjectKey() {}
	FLiveLinkSubjectKey(FName InSubjectName, FGuid InSource) : SubjectName(InSubjectName), Source(InSource) {}
	FLiveLinkSubjectKey(const FLiveLinkSubjectKey& Rhs) : SubjectName(Rhs.SubjectName), Source(Rhs.Source) {}
};

// Completely empty "source" that virtual subjects can hang off
struct FLiveLinkVirtualSubjectSource : public ILiveLinkSource
{
	virtual bool CanBeDisplayedInUI() const override { return false; }
	virtual void ReceiveClient(ILiveLinkClient* InClient, FGuid InSourceGuid) override {}
	virtual bool IsSourceStillValid() override { return true; }
	virtual bool RequestSourceShutdown() override { return true; }

	virtual FText GetSourceType() const override;
	virtual FText GetSourceMachineName() const override { return FText(); }
	virtual FText GetSourceStatus() const override { return FText(); }
};

class LIVELINK_API FLiveLinkClient : public ILiveLinkClient, public FTickableGameObject, public FGCObject
{
public:
	FLiveLinkClient() : LastValidationCheck(0.0), VirtualSubjectGuid(FGuid::NewGuid()) { AddVirtualSubjectSource(); }
	~FLiveLinkClient();

	// Begin FTickableGameObject implementation
	virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Always; }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual bool IsTickableInEditor() const override { return true; }
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(LiveLinkClient, STATGROUP_Tickables); }
	// End FTickableGameObject

	// Begin FGCObject implementation
	void AddReferencedObjects(FReferenceCollector & Collector);
	// End FGCObject

	// Remove the specified source from the live link client
	void RemoveSource(FGuid InEntryGuid);

	// Remover all sources from the live link client
	void RemoveAllSources();

	// ILiveLinkClient Interface

	virtual void AddSource(TSharedPtr<ILiveLinkSource> InSource) override;

	virtual void PushSubjectSkeleton(FGuid SourceGuid, FName SubjectName, const FLiveLinkRefSkeleton& RefSkeleton) override;

	virtual void ClearSubject(FName SubjectName) override;
	virtual void PushSubjectData(FGuid SourceGuid, FName SubjectName, const FLiveLinkFrameData& FrameData) override;
	// End ILiveLinkClient Interface

	// Add a new virtual subject to the client
	void AddVirtualSubject(FName NewVirtualSubjectName);

	virtual const FLiveLinkSubjectFrame* GetSubjectData(FName SubjectName) override;

	const TArray<FGuid>& GetSourceEntries() const { return SourceGuids; }

	// Get a list of currently active subjects
	TArray<FLiveLinkSubjectKey> GetSubjects();

	FText GetSourceTypeForEntry(FGuid InEntryGuid) const;
	FText GetMachineNameForEntry(FGuid InEntryGuid) const;
	FText GetEntryStatusForEntry(FGuid InEntryGuid) const;
	
	// Should the supplied source be shown in the source UI list 
	bool ShowSourceInUI(FGuid InEntryGuid) const;

	// Is the supplied source virtual
	bool IsVirtualSubject(const FLiveLinkSubjectKey& Subject) const;

	// Update an existing virtual subject with new settings
	void UpdateVirtualSubjectProperties(const FLiveLinkSubjectKey& Subject, const FLiveLinkVirtualSubject& VirtualSubject);

	// Get the settings of an exisitng virtual subject
	FLiveLinkVirtualSubject GetVirtualSubjectProperties(const FLiveLinkSubjectKey& SubjectKey) const;

	// Get interpolation settings for a source
	FLiveLinkInterpolationSettings* GetInterpolationSettingsForEntry(FGuid InEntryGuid);

	// Get full settings structure for source 
	ULiveLinkSourceSettings* GetSourceSettingsForEntry(FGuid InEntryGuid);

	void OnPropertyChanged(FGuid InEntryGuid, const FPropertyChangedEvent& PropertyChangedEvent);

	// Functions for managing sources changed delegate
	FDelegateHandle RegisterSourcesChangedHandle(const FSimpleMulticastDelegate::FDelegate& SourcesChanged);
	void UnregisterSourcesChangedHandle(FDelegateHandle Handle);

	// Functions for managing sources changed delegate
	FDelegateHandle RegisterSubjectsChangedHandle(const FSimpleMulticastDelegate::FDelegate& SubjectsChanged);
	void UnregisterSubjectsChangedHandle(FDelegateHandle Handle);

private:

	// Setup the source for virtual subjects
	void AddVirtualSubjectSource();

	// Remove the specified source (must be a valid index, function does no checking)
	void RemoveSourceInternal(int32 SourceIdx);

	// Get index of specified source
	int32 GetSourceIndexForGUID(FGuid InEntryGuid) const;

	// Get specified live link source
	TSharedPtr<ILiveLinkSource> GetSourceForGUID(FGuid InEntryGuid) const;

	// Test currently added sources to make sure they are still valid
	void ValidateSources();

	// Build subject data so that during the rest of the tick it can be read without
	// thread locking or mem copying
	void BuildThisTicksSubjectSnapshot();

	// Builds a FLiveLinkSubjectFrame for the supplied virtual subject out of data from the ActiveSubjectSnapshots
	void BuildVirtualSubjectFrame(FLiveLinkVirtualSubject& VirtualSubject, FLiveLinkSubjectFrame& SnapshotSubject);

	// Builds a new ref skeleton for a virtual subject
	FLiveLinkRefSkeleton BuildRefSkeletonForVirtualSubject(const FLiveLinkVirtualSubject& VirtualSubject);

	// Virtual Live Link Subjects (subjects that are build from multiple real subjects)
	TMap<FName, FLiveLinkVirtualSubject> VirtualSubjects;

	// Current streamed data for subjects
	TMap<FName, FLiveLinkSubject> LiveSubjectData;

	// Built snapshot of streamed subject data (updated once a tick)
	TMap<FName, FLiveLinkSubjectFrame> ActiveSubjectSnapshots;

	// Maintained array of names so that we dont have to repeatedly called GenerateKeyArray on ActiveSubjectSnapshots
	TArray<FName> ActiveSubjectNames;

	// Current Sources
	TArray<TSharedPtr<ILiveLinkSource>> Sources;
	TArray<FGuid>			 SourceGuids;
	TArray<ULiveLinkSourceSettings*> SourceSettings;

	// Sources that we are currently trying to remove
	TArray<TSharedPtr<ILiveLinkSource>>			 SourcesToRemove;

	// Cache last time we checked the validity of our sources 
	double LastValidationCheck;

	// Lock to stop multiple threads accessing the subject data map at the same time
	FCriticalSection SubjectDataAccessCriticalSection;

	// Delegate to notify interested parties when the client sources have changed
	FSimpleMulticastDelegate OnLiveLinkSourcesChanged;

	// Delegate to notify interested parties when the client subjects have changed
	FSimpleMulticastDelegate OnLiveLinkSubjectsChanged;

	// "source guid" for virtual subjects
	FGuid VirtualSubjectGuid;
};