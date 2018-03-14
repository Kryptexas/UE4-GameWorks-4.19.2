// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ILiveLinkModule.h"

#include "Features/IModularFeatures.h"
#include "IMotionController.h"

#include "LiveLinkMessageBusHeartbeatManager.h"

#include "LiveLinkClient.h"

/**
 * Implements the Messaging module.
 */

#define LOCTEXT_NAMESPACE "LiveLinkModule"


class FLiveLinkMotionController : public IMotionController
{
	// Internal structure for caching enumerated data
	struct FLiveLinkMotionControllerEnumeratedSource
	{
		// Subject key for talking to live link
		FLiveLinkSubjectKey SubjectKey;

		// MotionSource name for interacting with Motion Controller system
		FName MotionSource;

		FLiveLinkMotionControllerEnumeratedSource(const FLiveLinkSubjectKey& Key, FName MotionSourceName) : SubjectKey(Key), MotionSource(MotionSourceName) {}
	};

	// Built array of Live Link Sources to give to Motion Controller system
	TArray<FLiveLinkMotionControllerEnumeratedSource> EnumeratedSources;

public:
	FLiveLinkMotionController(FLiveLinkClient& InClient) : Client(InClient)
	{ 
		BuildSourceData();
		OnSubjectsChangedHandle = Client.RegisterSubjectsChangedHandle(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FLiveLinkMotionController::OnSubjectsChangedHandler));
		WildcardSource = FGuid::NewGuid();
	}

	~FLiveLinkMotionController()
	{
		Client.UnregisterSubjectsChangedHandle(OnSubjectsChangedHandle);
		OnSubjectsChangedHandle.Reset();
	}

	void RegisterController()
	{
		IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
	}

	void UnregisterController()
	{
		IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
	}

	virtual bool GetControllerOrientationAndPosition(const int32 ControllerIndex, const FName MotionSource, FRotator& OutOrientation, FVector& OutPosition, float WorldToMetersScale) const
	{
		FLiveLinkSubjectKey SubjectKey = GetSubjectKeyFromMotionSource(MotionSource);
		if (const FLiveLinkSubjectFrame* Frame = Client.GetSubjectData(SubjectKey.SubjectName))
		{
			if (Frame->Transforms.Num() > 0)
			{
				OutPosition = Frame->Transforms[0].GetLocation();
				OutOrientation = Frame->Transforms[0].GetRotation().Rotator();
				return true;
			}
		}
		return false;
	}

	float GetCustomParameterValue(const FName MotionSource, FName ParameterName, bool& bValueFound) const override
	{
		FLiveLinkSubjectKey SubjectKey = GetSubjectKeyFromMotionSource(MotionSource);
		if (const FLiveLinkSubjectFrame* Frame = Client.GetSubjectData(SubjectKey.SubjectName))
		{
			int32 ParamIndex = Frame->CurveKeyData.CurveNames.IndexOfByKey(ParameterName);
			if (ParamIndex != INDEX_NONE)
			{
				const FOptionalCurveElement& CurveElement = Frame->Curves[ParamIndex];
				if (CurveElement.IsValid())
				{
					bValueFound = true;
					return CurveElement.Value;
				}
			}
		}

		bValueFound = false;
		return 0.f;
	}

	virtual ETrackingStatus GetControllerTrackingStatus(const int32 ControllerIndex, const FName MotionSource) const override
	{
		FLiveLinkSubjectKey SubjectKey = GetSubjectKeyFromMotionSource(MotionSource);
		if (const FLiveLinkSubjectFrame* Frame = Client.GetSubjectData(SubjectKey.SubjectName))
		{
			return ETrackingStatus::Tracked;
		}
		return ETrackingStatus::NotTracked;
	}


	virtual FName GetMotionControllerDeviceTypeName() const override
	{
		static FName LiveLinkMotionControllerName(TEXT("LiveLinkMotionController"));
		return LiveLinkMotionControllerName;
	}

	// Builds cached source data for passing to motion controller system
	void BuildSourceData()
	{
		TArray<FLiveLinkSubjectKey> SubjectKeys = Client.GetSubjects();

		TMap<FGuid, TArray<FName>> LiveLinkSubjects;

		TArray<FName>& UniqueNames = LiveLinkSubjects.Add(WildcardSource);
		
		for (const FLiveLinkSubjectKey& Subject : SubjectKeys)
		{
			LiveLinkSubjects.FindOrAdd(Subject.Source).Add(Subject.SubjectName);
			UniqueNames.AddUnique(Subject.SubjectName);
		}

		TArray<FGuid> SourceGuids;
		LiveLinkSubjects.GenerateKeyArray(SourceGuids);

		typedef TPair<FGuid, FText> FHeaderEntry;
		TArray<FHeaderEntry> Headers;
		Headers.Reserve(SourceGuids.Num());

		for (const FGuid& Source : SourceGuids)
		{
			FText SourceName = (Source == WildcardSource) ? LOCTEXT("LiveLinkAnySource", "Any") : Client.GetSourceTypeForEntry(Source);
			Headers.Emplace(Source, SourceName);
		}

		{
			FGuid& CaptureWildcardSource = WildcardSource;
			Headers.Sort([CaptureWildcardSource](const FHeaderEntry& A, const FHeaderEntry& B) { return A.Key == CaptureWildcardSource || A.Value.CompareToCaseIgnored(B.Value) <= 0; });
		}

		//Build EnumeratedSources data
		EnumeratedSources.Reset();
		EnumeratedSources.Reserve(SubjectKeys.Num());

		for (const FHeaderEntry& Header : Headers)
		{
			TArray<FName> Subjects = LiveLinkSubjects.FindChecked(Header.Key);
			Subjects.Sort();
			for (FName Subject : Subjects)
			{
				FName FullName = *FString::Format(TEXT("{0} ({1})"), { Subject.ToString(), Header.Value.ToString() });
				EnumeratedSources.Emplace(FLiveLinkSubjectKey(Subject,Header.Key), FullName);
			}
		}
	}

	virtual void EnumerateSources(TArray<FMotionControllerSource>& Sources) const override
	{
		for (const FLiveLinkMotionControllerEnumeratedSource& Source : EnumeratedSources)
		{
			FMotionControllerSource SourceDesc(Source.MotionSource);
#if WITH_EDITOR
			static const FName LiveLinkCategoryName(TEXT("LiveLink"));
			SourceDesc.EditorCategory = LiveLinkCategoryName;
#endif
			Sources.Add(SourceDesc);
		}
	}

private:
	FLiveLinkSubjectKey GetSubjectKeyFromMotionSource(FName MotionSource) const
	{
		const FLiveLinkMotionControllerEnumeratedSource* EnumeratedSource = EnumeratedSources.FindByPredicate([&](const FLiveLinkMotionControllerEnumeratedSource& Item) { return Item.MotionSource == MotionSource; });
		if (EnumeratedSource)
		{
			return EnumeratedSource->SubjectKey;
		}
		return FLiveLinkSubjectKey(MotionSource,FGuid());
	}

	// Registered with the client and called when client's subjects change
	void OnSubjectsChangedHandler() { BuildSourceData(); }

	// Reference to the live link client
	FLiveLinkClient& Client;

	// Handle to delegate registered with client so we can update when subject state changes
	FDelegateHandle OnSubjectsChangedHandle;

	// Wildcard source, we don't care about the source itself, just the subject name
	FGuid WildcardSource;
};

class FLiveLinkModule
	: public ILiveLinkModule
{
public:
	FLiveLinkClient LiveLinkClient;
	FLiveLinkMotionController LiveLinkMotionController;

	FLiveLinkModule()
		: LiveLinkClient()
		, LiveLinkMotionController(LiveLinkClient)
	{}
	// IModuleInterface interface

	virtual void StartupModule() override
	{
		IModularFeatures::Get().RegisterModularFeature(FLiveLinkClient::ModularFeatureName, &LiveLinkClient);
		LiveLinkMotionController.RegisterController();
		// Create a HeartbeatManager Instance
		FHeartbeatManager::Get();
	}

	virtual void ShutdownModule() override
	{
		LiveLinkMotionController.UnregisterController();
		IModularFeatures::Get().UnregisterModularFeature(FLiveLinkClient::ModularFeatureName, &LiveLinkClient);
		delete FHeartbeatManager::Get();
	}

	virtual bool SupportsDynamicReloading() override
	{
		return false;
	}
};

IMPLEMENT_MODULE(FLiveLinkModule, LiveLink);

#undef LOCTEXT_NAMESPACE