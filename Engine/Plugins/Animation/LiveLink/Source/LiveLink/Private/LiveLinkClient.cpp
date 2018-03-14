// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "LiveLinkClient.h"
#include "ScopeLock.h"
#include "UObjectHash.h"
#include "LiveLinkSourceFactory.h"
#include "Guid.h"
#include "Package.h"

DEFINE_LOG_CATEGORY(LogLiveLink);

const double VALIDATE_SOURCES_TIME = 3.0; //How long should we wait between validation checks
const int32 MIN_FRAMES_TO_REMOVE = 5;

FLiveLinkCurveIntegrationData FLiveLinkCurveKey::UpdateCurveKey(const TArray<FLiveLinkCurveElement>& CurveElements)
{
	FLiveLinkCurveIntegrationData IntegrationData;

	int32 CurrentSize = CurveNames.Num();

	IntegrationData.CurveValues.AddDefaulted(CurrentSize);
	
	for(const FLiveLinkCurveElement& Elem : CurveElements)
	{
		int32 CurveIndex = CurveNames.IndexOfByKey(Elem.CurveName);
		if (CurveIndex == INDEX_NONE)
		{
			CurveIndex = CurveNames.Add(Elem.CurveName);
			IntegrationData.CurveValues.AddDefaulted();
		}
		IntegrationData.CurveValues[CurveIndex].SetValue(Elem.CurveValue);
	}
	IntegrationData.NumNewCurves = CurveNames.Num() - CurrentSize;

	return IntegrationData;
}

void BlendItem(const FTransform& A, const FTransform& B, FTransform& Output, float BlendWeight)
{
	const ScalarRegister ABlendWeight(1.0f - BlendWeight);
	const ScalarRegister BBlendWeight(BlendWeight);

	Output = A * ABlendWeight;
	Output.AccumulateWithShortestRotation(B, BBlendWeight);
	Output.NormalizeRotation();
}

void BlendItem(const FOptionalCurveElement& A, const FOptionalCurveElement& B, FOptionalCurveElement& Output, float BlendWeight)
{
	Output.Value = (A.Value * (1.0f - BlendWeight)) + (B.Value * BlendWeight);
	Output.bValid = A.bValid || B.bValid;
}

template<class Type>
void Blend(const TArray<Type>& A, const TArray<Type>& B, TArray<Type>& Output, float BlendWeight)
{
	check(A.Num() == B.Num());
	Output.SetNum(A.Num(), false);

	for (int32 BlendIndex = 0; BlendIndex < A.Num(); ++BlendIndex)
	{
		BlendItem(A[BlendIndex], B[BlendIndex], Output[BlendIndex], BlendWeight);
	}
}

void FLiveLinkSubject::AddFrame(const FLiveLinkFrameData& FrameData, FGuid FrameSource)
{
	LastModifier = FrameSource;

	FLiveLinkFrame* NewFrame = nullptr;

	if (CachedInterpolationSettings.bUseInterpolation)
	{
		if (FrameData.WorldTime.Time < LastReadTime)
		{
			//Gone back in time
			Frames.Reset();
			LastReadTime = 0;
			SubjectTimeOffset = FrameData.WorldTime.Offset;
		}

		if (Frames.Num() == 0)
		{
			Frames.AddDefaulted();
			NewFrame = &Frames[0];
			LastReadFrame = 0;
		}
		else
		{
			if (LastReadFrame > MIN_FRAMES_TO_REMOVE)
			{
				check(Frames.Num() > LastReadFrame);
				Frames.RemoveAt(0, LastReadFrame, false);
				LastReadFrame = 0;
			}

			int32 FrameIndex = Frames.Num() - 1;

			for (; FrameIndex >= 0; --FrameIndex)
			{
				if (Frames[FrameIndex].WorldTime.Time < FrameData.WorldTime.Time)
				{
					break;
				}
			}

			int32 NewFrameIndex = Frames.Insert(FLiveLinkFrame(), FrameIndex + 1);
			NewFrame = &Frames[NewFrameIndex];
		}
	}
	else
	{
		//No interpolation
		if (Frames.Num() > 1)
		{
			Frames.Reset();
		}

		if (Frames.Num() == 0)
		{
			Frames.AddDefaulted();
		}

		NewFrame = &Frames[0];

		LastReadTime = 0;
		LastReadFrame = 0;

		SubjectTimeOffset = FrameData.WorldTime.Offset;
	}

	FLiveLinkCurveIntegrationData IntegrationData = CurveKeyData.UpdateCurveKey(FrameData.CurveElements);

	check(NewFrame);
	NewFrame->Transforms = FrameData.Transforms;
	NewFrame->Curves = MoveTemp(IntegrationData.CurveValues);
	NewFrame->MetaData = FrameData.MetaData;
	NewFrame->WorldTime = FrameData.WorldTime;

	// update existing curves
	if (IntegrationData.NumNewCurves > 0)
	{
		for (FLiveLinkFrame& Frame : Frames)
		{
			Frame.ExtendCurveData(IntegrationData.NumNewCurves);
		}
	}
}


void FLiveLinkSubject::BuildInterpolatedFrame(const double InSeconds, FLiveLinkSubjectFrame& OutFrame)
{
	OutFrame.RefSkeleton = RefSkeleton;
	OutFrame.RefSkeletonGuid = RefSkeletonGuid;
	OutFrame.CurveKeyData = CurveKeyData;

	OutFrame.Transforms.Reset();
	OutFrame.Curves.Reset();

	if (!CachedInterpolationSettings.bUseInterpolation)
	{
		OutFrame.Transforms = Frames.Last().Transforms;
		OutFrame.Curves = Frames.Last().Curves;
		OutFrame.MetaData = Frames.Last().MetaData;
		LastReadTime = Frames.Last().WorldTime.Time;
		LastReadFrame = Frames.Num()-1;
	}
	else
	{
		LastReadTime = (InSeconds - SubjectTimeOffset) - CachedInterpolationSettings.InterpolationOffset;

		bool bBuiltFrame = false;

		for (int32 FrameIndex = Frames.Num() - 1; FrameIndex >= 0; --FrameIndex)
		{
			if (Frames[FrameIndex].WorldTime.Time < LastReadTime)
			{
				//Found Start frame

				if (FrameIndex == Frames.Num() - 1)
				{
					LastReadFrame = FrameIndex;
					OutFrame.Transforms = Frames[FrameIndex].Transforms;
					OutFrame.Curves = Frames[FrameIndex].Curves;
					OutFrame.MetaData = Frames[FrameIndex].MetaData;
					bBuiltFrame = true;
					break;
				}
				else
				{
					LastReadFrame = FrameIndex;
					const FLiveLinkFrame& PreFrame = Frames[FrameIndex];
					const FLiveLinkFrame& PostFrame = Frames[FrameIndex + 1];

					// Calc blend weight (Amount through frame gap / frame gap) 
					const float BlendWeight = (LastReadTime - PreFrame.WorldTime.Time) / (PostFrame.WorldTime.Time - PreFrame.WorldTime.Time);

					Blend(PreFrame.Transforms, PostFrame.Transforms, OutFrame.Transforms, BlendWeight);
					Blend(PreFrame.Curves, PostFrame.Curves, OutFrame.Curves, BlendWeight);
					// MetaData doesn't interpolate so use the PreFrame
					OutFrame.MetaData = PreFrame.MetaData;

					bBuiltFrame = true;
					break;
				}
			}
		}

		if (!bBuiltFrame)
		{
			LastReadFrame = 0;
			// Failed to find an interp point so just take earliest frame
			OutFrame.Transforms = Frames[0].Transforms;
			OutFrame.Curves = Frames[0].Curves;
			OutFrame.MetaData = Frames[0].MetaData;
		}
	}
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
			Sources.RemoveAtSwap(ToRemove[Idx],1,false);
		}
	}
}

void FLiveLinkClient::Tick(float DeltaTime)
{
	if (LastValidationCheck < FPlatformTime::Seconds() - VALIDATE_SOURCES_TIME)
	{
		ValidateSources();
	}

	BuildThisTicksSubjectSnapshot();
}

void FLiveLinkClient::AddReferencedObjects(FReferenceCollector & Collector)
{
	for (const ULiveLinkSourceSettings* Settings : SourceSettings)
	{
		Collector.AddReferencedObject(Settings);
	}
}

void FLiveLinkClient::ValidateSources()
{
	bool bSourcesChanged = false;
	for (int32 SourceIdx = Sources.Num() - 1; SourceIdx >= 0; --SourceIdx)
	{
		if (!Sources[SourceIdx]->IsSourceStillValid())
		{
			RemoveSourceInternal(SourceIdx);

			bSourcesChanged = true;
		}
	}

	for (int32 SourceIdx = SourcesToRemove.Num()-1; SourceIdx >= 0; --SourceIdx)
	{
		if (SourcesToRemove[SourceIdx]->RequestSourceShutdown())
		{
			SourcesToRemove.RemoveAtSwap(SourceIdx, 1, false);
		}
	}

	LastValidationCheck = FPlatformTime::Seconds();

	if (bSourcesChanged)
	{
		OnLiveLinkSourcesChanged.Broadcast();
	}
}

void FLiveLinkClient::BuildThisTicksSubjectSnapshot()
{
	const int32 PreviousSize = ActiveSubjectSnapshots.Num();

	TArray<FName> OldSubjectSnapshotNames;
	ActiveSubjectSnapshots.GenerateKeyArray(OldSubjectSnapshotNames);
	
	const double CurrentInterpTime = FPlatformTime::Seconds();	// Set this up once, every subject
																// uses the same time

	{
		FScopeLock Lock(&SubjectDataAccessCriticalSection);

		for (TPair<FName, FLiveLinkSubject>& SubjectPair : LiveSubjectData)
		{
			const FName SubjectName = SubjectPair.Key;
			OldSubjectSnapshotNames.RemoveSingleSwap(SubjectName, false);

			FLiveLinkSubject& SourceSubject = SubjectPair.Value;

			FLiveLinkInterpolationSettings* SubjectInterpolationSettings = GetInterpolationSettingsForEntry(SourceSubject.LastModifier);
			if (SubjectInterpolationSettings)
			{
				SourceSubject.CachedInterpolationSettings = *SubjectInterpolationSettings;
			}

			if (SourceSubject.Frames.Num() > 0)
			{
				FLiveLinkSubjectFrame* SnapshotSubject = ActiveSubjectSnapshots.Find(SubjectName);
				if (!SnapshotSubject)
				{
					ActiveSubjectNames.Add(SubjectName);
					SnapshotSubject = &ActiveSubjectSnapshots.Add(SubjectName);
				}

				SourceSubject.BuildInterpolatedFrame(CurrentInterpTime, *SnapshotSubject);
			}
		}
	}

	//Now that ActiveSubjectSnapshots is up to date we now need to build the virtual subject data
	for (TPair<FName, FLiveLinkVirtualSubject>& SubjectPair : VirtualSubjects)
	{
		if(SubjectPair.Value.GetSubjects().Num() > 0)
		{
			const FName SubjectName = SubjectPair.Key;
			OldSubjectSnapshotNames.RemoveSingleSwap(SubjectName, false);

			FLiveLinkSubjectFrame& SnapshotSubject = ActiveSubjectSnapshots.FindOrAdd(SubjectName);

			BuildVirtualSubjectFrame(SubjectPair.Value, SnapshotSubject);
		}
	}

	if (PreviousSize != ActiveSubjectSnapshots.Num() || OldSubjectSnapshotNames.Num() > 0)
	{
		//Have either added or removed a subject, must signal update
		OnLiveLinkSubjectsChanged.Broadcast();
	}

	for (FName SubjectName : OldSubjectSnapshotNames)
	{
		ActiveSubjectSnapshots.Remove(SubjectName);
		ActiveSubjectNames.RemoveSingleSwap(SubjectName, false);
	}
}

void FLiveLinkClient::BuildVirtualSubjectFrame(FLiveLinkVirtualSubject& VirtualSubject, FLiveLinkSubjectFrame& SnapshotSubject)
{
	VirtualSubject.BuildRefSkeletonForVirtualSubject(ActiveSubjectSnapshots, ActiveSubjectNames);

	SnapshotSubject.RefSkeleton = VirtualSubject.GetRefSkeleton();
	SnapshotSubject.CurveKeyData = VirtualSubject.CurveKeyData;

	SnapshotSubject.Transforms.Reset(SnapshotSubject.RefSkeleton.GetBoneNames().Num());
	SnapshotSubject.Transforms.Add(FTransform::Identity);
	SnapshotSubject.MetaData.StringMetaData.Empty();
	for (FName SubjectName : VirtualSubject.Subjects)
	{
		FLiveLinkSubjectFrame& SubjectFrame = ActiveSubjectSnapshots.FindChecked(SubjectName);
		SnapshotSubject.Transforms.Append(SubjectFrame.Transforms);
		for (const auto& MetaDatum : SubjectFrame.MetaData.StringMetaData)
		{
			FName QualifiedKey = FName(*(SubjectName.ToString() + MetaDatum.Key.ToString()));
			SnapshotSubject.MetaData.StringMetaData.Emplace(SubjectName, MetaDatum.Value);
		}
	}
}

void FLiveLinkClient::AddVirtualSubject(FName NewVirtualSubjectName)
{
	VirtualSubjects.Add(NewVirtualSubjectName);
}

void FLiveLinkClient::AddSource(TSharedPtr<ILiveLinkSource> InSource)
{
	Sources.Add(InSource);
	SourceGuids.Add(FGuid::NewGuid());

	UClass* CustomSettingsClass = InSource->GetCustomSettingsClass();

	if (CustomSettingsClass && !CustomSettingsClass->IsChildOf<ULiveLinkSourceSettings>())
	{
		UE_LOG(LogLiveLink, Warning, TEXT("Custom Setting Failure: Source '%s' settings class '%s' does not derive from ULiveLinkSourceSettings"), *InSource->GetSourceType().ToString(), *CustomSettingsClass->GetName());
		CustomSettingsClass = nullptr;
	}

	UClass* SettingsClass = CustomSettingsClass ? CustomSettingsClass : ULiveLinkSourceSettings::StaticClass();
	ULiveLinkSourceSettings* NewSettings = NewObject<ULiveLinkSourceSettings>(GetTransientPackage(), SettingsClass);

	SourceSettings.Add(NewSettings);
	
	InSource->ReceiveClient(this, SourceGuids.Last());
	InSource->InitializeSettings(NewSettings);
}

void FLiveLinkClient::AddVirtualSubjectSource()
{
	SourceGuids.Add(VirtualSubjectGuid);
	Sources.Add(MakeShared<FLiveLinkVirtualSubjectSource>());

	ULiveLinkSourceSettings* NewSettings = NewObject<ULiveLinkSourceSettings>(GetTransientPackage());
	SourceSettings.Add(NewSettings);
}

void FLiveLinkClient::RemoveSourceInternal(int32 SourceIdx)
{
	Sources.RemoveAtSwap(SourceIdx, 1, false);
	SourceGuids.RemoveAtSwap(SourceIdx, 1, false);
	SourceSettings.RemoveAtSwap(SourceIdx, 1, false);
}

void FLiveLinkClient::RemoveSource(FGuid InEntryGuid)
{
	LastValidationCheck = 0.0; //Force validation check next frame
	int32 SourceIdx = GetSourceIndexForGUID(InEntryGuid);
	if (SourceIdx != INDEX_NONE)
	{
		SourcesToRemove.Add(Sources[SourceIdx]);
		RemoveSourceInternal(SourceIdx);
		OnLiveLinkSourcesChanged.Broadcast();
	}
}

void FLiveLinkClient::RemoveAllSources()
{
	LastValidationCheck = 0.0; //Force validation check next frame
	SourcesToRemove = Sources;
	Sources.Reset();
	SourceGuids.Reset();
	SourceSettings.Reset();

	AddVirtualSubjectSource();
	OnLiveLinkSourcesChanged.Broadcast();
}

void FLiveLinkClient::PushSubjectSkeleton(FGuid SourceGuid, FName SubjectName, const FLiveLinkRefSkeleton& RefSkeleton)
{
	FScopeLock Lock(&SubjectDataAccessCriticalSection);
	
	if (FLiveLinkSubject* Subject = LiveSubjectData.Find(SubjectName))
	{
		Subject->Frames.Reset();
		Subject->SetRefSkeleton(RefSkeleton);
		Subject->LastModifier = SourceGuid;
	}
	else
	{
		LiveSubjectData.Emplace(SubjectName, FLiveLinkSubject(RefSkeleton)).LastModifier = SourceGuid;
	}
}

void FLiveLinkClient::ClearSubject(FName SubjectName)
{
	FScopeLock Lock(&SubjectDataAccessCriticalSection);

	LiveSubjectData.Remove(SubjectName);
}

void FLiveLinkClient::PushSubjectData(FGuid SourceGuid, FName SubjectName, const FLiveLinkFrameData& FrameData)
{
	FScopeLock Lock(&SubjectDataAccessCriticalSection);

	if (FLiveLinkSubject* Subject = LiveSubjectData.Find(SubjectName))
	{
		Subject->AddFrame(FrameData, SourceGuid);
	}
}

const FLiveLinkSubjectFrame* FLiveLinkClient::GetSubjectData(FName SubjectName)
{
	if (FLiveLinkSubjectFrame* Subject = ActiveSubjectSnapshots.Find(SubjectName))
	{
		return Subject;
	}
	return nullptr;
}

TArray<FLiveLinkSubjectKey> FLiveLinkClient::GetSubjects()
{
	TArray<FLiveLinkSubjectKey> SubjectEntries;
	{
		FScopeLock Lock(&SubjectDataAccessCriticalSection);
		
		SubjectEntries.Reserve(LiveSubjectData.Num() + VirtualSubjects.Num());

		for (const TPair<FName, FLiveLinkSubject>& LiveSubject : LiveSubjectData)
		{
			SubjectEntries.Emplace(LiveSubject.Key, LiveSubject.Value.LastModifier);
		}
	}

	for (TPair<FName, FLiveLinkVirtualSubject>& VirtualSubject : VirtualSubjects)
	{
		const int32 NewItem = SubjectEntries.Emplace(VirtualSubject.Key, VirtualSubjectGuid);
	}

	return SubjectEntries;
}

int32 FLiveLinkClient::GetSourceIndexForGUID(FGuid InEntryGuid) const
{
	return SourceGuids.IndexOfByKey(InEntryGuid);
}

TSharedPtr<ILiveLinkSource> FLiveLinkClient::GetSourceForGUID(FGuid InEntryGuid) const
{
	int32 Idx = GetSourceIndexForGUID(InEntryGuid);
	return Idx != INDEX_NONE ? Sources[Idx] : nullptr;
}

FText FLiveLinkClient::GetSourceTypeForEntry(FGuid InEntryGuid) const
{
	TSharedPtr<ILiveLinkSource> Source = GetSourceForGUID(InEntryGuid);
	if (Source.IsValid())
	{
		return Source->GetSourceType();
	}
	return FText(NSLOCTEXT("TempLocTextLiveLink","InvalidSourceType", "Invalid Source Type"));
}

FText FLiveLinkClient::GetMachineNameForEntry(FGuid InEntryGuid) const
{
	TSharedPtr<ILiveLinkSource> Source = GetSourceForGUID(InEntryGuid);
	if (Source.IsValid())
	{
		return Source->GetSourceMachineName();
	}
	return FText(NSLOCTEXT("TempLocTextLiveLink","InvalidSourceMachineName", "Invalid Source Machine Name"));
}

bool FLiveLinkClient::ShowSourceInUI(FGuid InEntryGuid) const
{
	TSharedPtr<ILiveLinkSource> Source = GetSourceForGUID(InEntryGuid);
	if (Source.IsValid())
	{
		return Source->CanBeDisplayedInUI();
	}
	return false;
}

bool FLiveLinkClient::IsVirtualSubject(const FLiveLinkSubjectKey& Subject) const
{
	return Subject.Source == VirtualSubjectGuid && VirtualSubjects.Contains(Subject.SubjectName);
}

FText FLiveLinkClient::GetEntryStatusForEntry(FGuid InEntryGuid) const
{
	TSharedPtr<ILiveLinkSource> Source = GetSourceForGUID(InEntryGuid);
	if (Source.IsValid())
	{
		return Source->GetSourceStatus();
	}
	return FText(NSLOCTEXT("TempLocTextLiveLink","InvalidSourceStatus", "Invalid Source Status"));
}

FLiveLinkInterpolationSettings* FLiveLinkClient::GetInterpolationSettingsForEntry(FGuid InEntryGuid)
{
	const int32 SourceIndex = GetSourceIndexForGUID(InEntryGuid);
	return (SourceIndex != INDEX_NONE) ? &SourceSettings[SourceIndex]->InterpolationSettings : nullptr;
}

ULiveLinkSourceSettings* FLiveLinkClient::GetSourceSettingsForEntry(FGuid InEntryGuid)
{
	const int32 SourceIndex = GetSourceIndexForGUID(InEntryGuid);
	return (SourceIndex != INDEX_NONE) ? SourceSettings[SourceIndex] : nullptr;
}

void FLiveLinkClient::UpdateVirtualSubjectProperties(const FLiveLinkSubjectKey& Subject, const FLiveLinkVirtualSubject& VirtualSubject)
{
	if (Subject.Source == VirtualSubjectGuid)
	{
		FLiveLinkVirtualSubject& ExistingVirtualSubject = VirtualSubjects.FindOrAdd(Subject.SubjectName);
		ExistingVirtualSubject = VirtualSubject;
		ExistingVirtualSubject.InvalidateSubjectGuids();
	}
}

FLiveLinkVirtualSubject FLiveLinkClient::GetVirtualSubjectProperties(const FLiveLinkSubjectKey& SubjectKey) const
{
	check(SubjectKey.Source == VirtualSubjectGuid);
	
	return VirtualSubjects.FindChecked(SubjectKey.SubjectName);
}

void FLiveLinkClient::OnPropertyChanged(FGuid InEntryGuid, const FPropertyChangedEvent& PropertyChangedEvent)
{
	const int32 SourceIndex = GetSourceIndexForGUID(InEntryGuid);
	if (SourceIndex != INDEX_NONE)
	{
		Sources[SourceIndex]->OnSettingsChanged(SourceSettings[SourceIndex], PropertyChangedEvent);
	}
}

FDelegateHandle FLiveLinkClient::RegisterSourcesChangedHandle(const FSimpleMulticastDelegate::FDelegate& SourcesChanged)
{
	return OnLiveLinkSourcesChanged.Add(SourcesChanged);
}

void FLiveLinkClient::UnregisterSourcesChangedHandle(FDelegateHandle Handle)
{
	OnLiveLinkSourcesChanged.Remove(Handle);
}

FDelegateHandle FLiveLinkClient::RegisterSubjectsChangedHandle(const FSimpleMulticastDelegate::FDelegate& SubjectsChanged)
{
	return OnLiveLinkSubjectsChanged.Add(SubjectsChanged);
}

void FLiveLinkClient::UnregisterSubjectsChangedHandle(FDelegateHandle Handle)
{
	OnLiveLinkSubjectsChanged.Remove(Handle);
}

FText FLiveLinkVirtualSubjectSource::GetSourceType() const
{
	return NSLOCTEXT("TempLocTextLiveLink", "LiveLinkVirtualSubjectName", "Virtual Subjects");
}
