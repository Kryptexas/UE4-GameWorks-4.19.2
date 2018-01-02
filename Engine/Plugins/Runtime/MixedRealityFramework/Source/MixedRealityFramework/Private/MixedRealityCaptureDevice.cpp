// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MixedRealityCaptureDevice.h"
#include "MediaPlayer.h"
#include "MediaCaptureSupport.h" // for EnumerateVideoCaptureDevices()
#include "IMediaCaptureSupport.h" // for FMediaCaptureDeviceInfo
#include "MixedRealityCaptureComponent.h" // for LogMixedReality
#include "MixedRealitySettings.h"
#include "IMediaEventSink.h" // for EMediaEvent


#define LOCTEXT_NAMESPACE "MixedRealityCaptureDevice"

/* FMRCaptureDeviceIndex
 *****************************************************************************/

FMRCaptureDeviceIndex::FMRCaptureDeviceIndex()
	: StreamIndex(0)
	, FormatIndex(0)
{}

FMRCaptureDeviceIndex::FMRCaptureDeviceIndex::FMRCaptureDeviceIndex(UMediaPlayer* MediaPlayer)
	: StreamIndex(0)
	, FormatIndex(0)
{
	if (MediaPlayer != nullptr)
	{
		DeviceURL   = MediaPlayer->GetUrl();
		StreamIndex = MediaPlayer->GetSelectedTrack(EMediaPlayerTrack::Video);
		FormatIndex = MediaPlayer->GetTrackFormat(EMediaPlayerTrack::Video, StreamIndex);
	}
}

FMRCaptureDeviceIndex::FMRCaptureDeviceIndex(FMediaCaptureDeviceInfo& DeviceInfo)
	: DeviceURL(DeviceInfo.Url)
	, StreamIndex(0)
	, FormatIndex(0)
{}

bool FMRCaptureDeviceIndex::IsSet(UMediaPlayer* MediaPlayer) const
{
	return MediaPlayer && (MediaPlayer->GetSelectedTrack(EMediaPlayerTrack::Video) == StreamIndex) && 
		(MediaPlayer->GetTrackFormat(EMediaPlayerTrack::Video, StreamIndex) == FormatIndex) && MediaPlayer->GetUrl() == DeviceURL;
}

bool FMRCaptureDeviceIndex::IsDeviceUrlValid() const
{
	if (!DeviceURL.IsEmpty())
	{
		TArray<FMediaCaptureDeviceInfo> ActiveDevices;
		MediaCaptureSupport::EnumerateVideoCaptureDevices(ActiveDevices);

		for (const FMediaCaptureDeviceInfo& ConnectedDevice : ActiveDevices)
		{
			if (ConnectedDevice.Url == DeviceURL)
			{
				return true;
			}
		}
	}
	return false;
}

/* UMRCaptureDeviceLibrary
 *****************************************************************************/

UMRCaptureDeviceLibrary::UMRCaptureDeviceLibrary(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}


void UMRCaptureDeviceLibrary::GetFeedDescription(UMediaPlayer* MediaPlayer, FText& DeviceName, FText& FormatDesc)
{
	FormatDesc = LOCTEXT("NoCaptureFormat", "<NO FORMAT SELECTED>");

	if (!MediaPlayer || MediaPlayer->GetUrl().IsEmpty())
	{
		DeviceName = LOCTEXT("NoCaptureDevice", "<NO DEVICE OPEN>");
	}
	else
	{
		TArray<FMediaCaptureDeviceInfo> ActiveDevices;
		MediaCaptureSupport::EnumerateVideoCaptureDevices(ActiveDevices);

		DeviceName = LOCTEXT("NotAVideoCaptureDevice", "<INVALID VIDEO CAPTURE DEVICE>");
		for (const FMediaCaptureDeviceInfo& ConnectedDevice : ActiveDevices)
		{
			if (ConnectedDevice.Url == MediaPlayer->GetUrl())
			{
				DeviceName = ConnectedDevice.DisplayName;
				break;
			}
		}

		const int32 SelectedTrack = MediaPlayer->GetSelectedTrack(EMediaPlayerTrack::Video);
		const int32 SelectedFormat = (SelectedTrack >= 0) ? MediaPlayer->GetTrackFormat(EMediaPlayerTrack::Video, SelectedTrack) : INDEX_NONE;

		if (SelectedTrack >= 0 && SelectedFormat >= 0)
		{
			const FIntPoint Dim = MediaPlayer->GetVideoTrackDimensions(SelectedTrack, SelectedFormat);
			const float FrameRate = MediaPlayer->GetVideoTrackFrameRate(SelectedTrack, SelectedFormat);
			const TRange<float> FrameRates = MediaPlayer->GetVideoTrackFrameRates(SelectedTrack, SelectedFormat);
			const FString Type = MediaPlayer->GetVideoTrackType(SelectedTrack, SelectedFormat);

			FFormatNamedArguments Arguments;
			Arguments.Add(TEXT("DimX"), FText::AsNumber(Dim.X));
			Arguments.Add(TEXT("DimY"), FText::AsNumber(Dim.Y));
			Arguments.Add(TEXT("Fps"), FText::AsNumber(FrameRate));
			Arguments.Add(TEXT("Type"), FText::FromString(Type));

			if (FrameRates.IsDegenerate() && (FrameRates.GetLowerBoundValue() == FrameRate))
			{
				FormatDesc = FText::Format(LOCTEXT("TrackFormatMenuVideoFormat", "{Type} {DimX}x{DimY} {Fps} fps"), Arguments);
			}
			else
			{
				Arguments.Add(TEXT("FpsLower"), FText::AsNumber(FrameRates.GetLowerBoundValue()));
				Arguments.Add(TEXT("FpsUpper"), FText::AsNumber(FrameRates.GetUpperBoundValue()));

				FormatDesc = FText::Format(LOCTEXT("TrackFormatMenuVideoFormat", "{Type} {DimX}x{DimY} {Fps} [{FpsLower}-{FpsUpper}] fps"), Arguments);
			}
		}
	}
}

TArray<FMRCaptureDeviceIndex> UMRCaptureDeviceLibrary::SortAvailableFormats(UMediaPlayer* MediaPlayer)
{
	const UMixedRealityFrameworkSettings* MRSettings = GetDefault<UMixedRealityFrameworkSettings>();
	const FString DesiredFormat = MRSettings->DesiredCaptureFormat;
	const float DesiredAspectRatio = MRSettings->DesiredCaptureAspectRatio; 
	const int32 DesiredResolution = MRSettings->DesiredCaptureResolution;
	
	const int32 DesiredFormatIndex = MRSettings->DesiredCaptureDeviceFormatIndex;

	TArray<FMRCaptureDeviceIndex> AvailableFormats;
	if (MediaPlayer != nullptr && !MediaPlayer->GetUrl().IsEmpty())
	{
		const int32 NumTracks = MediaPlayer->GetNumTracks(EMediaPlayerTrack::Video);
		for (int32 Track = 0; Track < NumTracks; ++Track)
		{
			const int32 FormatCount = MediaPlayer->GetNumTrackFormats(EMediaPlayerTrack::Video, Track);

			int32 BlockIndex = AvailableFormats.AddDefaulted(FormatCount);
			for (int32 FormatIndex = 0; FormatIndex < FormatCount; ++FormatIndex)
			{
				FMRCaptureDeviceIndex& Format = AvailableFormats[BlockIndex + FormatIndex];
				Format.DeviceURL   = MediaPlayer->GetUrl();
				Format.StreamIndex = Track;
				Format.FormatIndex = FormatIndex;
			}
		}

		// for some reason their listed in reverse order in the editor
		const int32 DesiredTrackIndex = NumTracks - MRSettings->DesiredCaptureDeviceTrackIndex - 1;

		AvailableFormats.Sort([MediaPlayer, DesiredFormat, DesiredAspectRatio, DesiredResolution, DesiredTrackIndex, DesiredFormatIndex](const FMRCaptureDeviceIndex& A, const FMRCaptureDeviceIndex& B)->bool
			// should A come before B?
			{
				if (DesiredTrackIndex != INDEX_NONE)
				{
					const bool bAMatchesTrack = (A.StreamIndex == DesiredTrackIndex);
					const bool bBMatchesTrack = (B.StreamIndex == DesiredTrackIndex);

					if (bAMatchesTrack != bBMatchesTrack)
					{
						return bAMatchesTrack;
					}
					else if (bAMatchesTrack && DesiredFormatIndex != INDEX_NONE)
					{
						const bool bAMatchesFormat = (A.FormatIndex == DesiredFormatIndex);
						const bool bBMatchesFormat = (B.FormatIndex == DesiredFormatIndex);

						if (bAMatchesFormat != bBMatchesFormat)
						{
							return bAMatchesFormat;
						}
					}
				}

				if (!DesiredFormat.IsEmpty())
				{
					const FString A_Format = MediaPlayer->GetVideoTrackType(A.StreamIndex, A.FormatIndex);
					const bool bAMatchesFormat = (A_Format == DesiredFormat);
					const FString B_Format = MediaPlayer->GetVideoTrackType(B.StreamIndex, B.FormatIndex);
					const bool bBMatchesFormat = (B_Format == DesiredFormat);

					if (bAMatchesFormat != bBMatchesFormat)
					{
						return bAMatchesFormat;
					}
				}

				const float A_AspectRatio = MediaPlayer->GetVideoTrackAspectRatio(A.StreamIndex, A.FormatIndex);
				const bool bAMatchesAspect = FMath::IsNearlyEqual(A_AspectRatio, DesiredAspectRatio);
				const float B_AspectRatio = MediaPlayer->GetVideoTrackAspectRatio(B.StreamIndex, B.FormatIndex);
				const bool bBMatchesAspect = FMath::IsNearlyEqual(B_AspectRatio, DesiredAspectRatio);

				// prioritize matching the aspect ratio
				if (bAMatchesAspect != bBMatchesAspect)
				{
					return bAMatchesAspect;
				}

				const FIntPoint A_Dim = MediaPlayer->GetVideoTrackDimensions(A.StreamIndex, A.FormatIndex);
				const bool bAMatchesRes = (A_Dim.Y >= DesiredResolution);
				const bool bAMatchesResExact = A_Dim.Y == DesiredResolution;
				const FIntPoint B_Dim = MediaPlayer->GetVideoTrackDimensions(B.StreamIndex, B.FormatIndex);
				const bool bBMatchesRes = (B_Dim.Y >= DesiredResolution);
				const bool bBMatchesResExact = B_Dim.Y == DesiredResolution;

				// next, order formats matching the desired resolution (equal and above)
				if (bAMatchesRes != bBMatchesRes)
				{
					return bAMatchesRes;
				}
				else if (!bAMatchesRes)
				{
					const int32 A_ScreenArea = A_Dim.X * A_Dim.Y;
					const int32 B_ScreenArea = B_Dim.X * B_Dim.Y;
					// if both resolutions are under what's desired, order them by screen coverage
					return A_ScreenArea > B_ScreenArea;
				}
				else if ((bAMatchesResExact || bBMatchesResExact) && (A_Dim.Y != B_Dim.Y))
				{
					return bAMatchesResExact;
				}

				const TRange<float> A_FrameRateRange = MediaPlayer->GetVideoTrackFrameRates(A.StreamIndex, A.FormatIndex);
				float A_FrameRate = MediaPlayer->GetVideoTrackFrameRate(A.StreamIndex, A.FormatIndex);
				if (!A_FrameRateRange.IsDegenerate() || A_FrameRateRange.GetLowerBoundValue() != A_FrameRate)
				{
					A_FrameRate = A_FrameRateRange.GetUpperBoundValue();
				}

				const TRange<float> B_FrameRateRange = MediaPlayer->GetVideoTrackFrameRates(B.StreamIndex, B.FormatIndex);
				float B_FrameRate = MediaPlayer->GetVideoTrackFrameRate(B.StreamIndex, B.FormatIndex);
				if (!B_FrameRateRange.IsDegenerate() || B_FrameRateRange.GetLowerBoundValue() != B_FrameRate)
				{
					B_FrameRate = B_FrameRateRange.GetUpperBoundValue();
				}

				// lastly, favor higher frame rates
				if (A_FrameRate != B_FrameRate)
				{
					return A_FrameRate > B_FrameRate;
				}
				if (A_FrameRateRange.GetLowerBoundValue() != B_FrameRateRange.GetLowerBoundValue())
				{
					return A_FrameRateRange.GetLowerBoundValue() > B_FrameRateRange.GetLowerBoundValue();
				}

				// maintain order if they're identical
				if (A.StreamIndex != B.StreamIndex)
				{
					return A.StreamIndex > B.StreamIndex;
				}
				return A.FormatIndex < B.FormatIndex;
			}
		);
	}
	else
	{
		UE_LOG(LogMixedReality, Warning, TEXT("Invalid media player for query - a valid, open capture feed is required for this query."));
	}
	return AvailableFormats;
}


/* FLatentPlayMRCaptureFeedAction
*****************************************************************************/

struct FLatentPlayMRCaptureFeedAction : public FTickableGameObject, public TSharedFromThis<FLatentPlayMRCaptureFeedAction>
{
public:
	static TSharedRef<FLatentPlayMRCaptureFeedAction> Create(UAsyncTask_OpenMRCaptureFeedBase* Owner);
	static TSharedPtr<FLatentPlayMRCaptureFeedAction> FindActiveAction(UMediaPlayer* MediaPlayer);
	static void FreeAction(UMediaPlayer* MediaPlayer);

	virtual ~FLatentPlayMRCaptureFeedAction();
	UAsyncTask_OpenMRCaptureFeedBase* GetOwner() const { return Owner; }

public:
	//~ FTickableObjectBase interface

	virtual bool IsTickable() const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	FLatentPlayMRCaptureFeedAction(UAsyncTask_OpenMRCaptureFeedBase* Owner);
	
	void HandleMediaPlayerMediaEvent(EMediaEvent Event);
	void Destroy();

	UAsyncTask_OpenMRCaptureFeedBase* Owner;
	FDelegateHandle OnMediaEventBinding;

	static TMap< UMediaPlayer*, TSharedPtr<FLatentPlayMRCaptureFeedAction> > ActiveAsyncActions;
};
TMap< UMediaPlayer*, TSharedPtr<FLatentPlayMRCaptureFeedAction> > FLatentPlayMRCaptureFeedAction::ActiveAsyncActions;


TSharedRef<FLatentPlayMRCaptureFeedAction> FLatentPlayMRCaptureFeedAction::Create(UAsyncTask_OpenMRCaptureFeedBase* InOwner)
{
	UMediaPlayer* MediaPlayer = InOwner->MediaPlayer;

	TSharedPtr<FLatentPlayMRCaptureFeedAction> ExistingAction = FLatentPlayMRCaptureFeedAction::FindActiveAction(MediaPlayer);
	if (ExistingAction.IsValid() && ExistingAction->GetOwner() == InOwner)
	{
		return ExistingAction.ToSharedRef();
	}
	else
	{
		FreeAction(MediaPlayer);

		TSharedRef<FLatentPlayMRCaptureFeedAction> NewAction = MakeShareable(new FLatentPlayMRCaptureFeedAction(InOwner));
		ActiveAsyncActions.Add(MediaPlayer, NewAction);

		return NewAction;
	}
}

TSharedPtr<FLatentPlayMRCaptureFeedAction> FLatentPlayMRCaptureFeedAction::FindActiveAction(UMediaPlayer* MediaPlayer)
{
	TSharedPtr<FLatentPlayMRCaptureFeedAction> ActiveAction;
	if (TSharedPtr<FLatentPlayMRCaptureFeedAction>* ExistingAction = ActiveAsyncActions.Find(MediaPlayer))
	{
		ActiveAction = *ExistingAction;
	}
	return ActiveAction;
}

void FLatentPlayMRCaptureFeedAction::FreeAction(UMediaPlayer* MediaPlayer)
{
	TSharedPtr<FLatentPlayMRCaptureFeedAction> ExistingAction = FLatentPlayMRCaptureFeedAction::FindActiveAction(MediaPlayer);
	if (ExistingAction.IsValid())
	{
		MediaPlayer->OnMediaEvent().Remove(ExistingAction->OnMediaEventBinding);

		ActiveAsyncActions.Remove(MediaPlayer);
		ExistingAction->GetOwner()->CleanUp();
	}
}

FLatentPlayMRCaptureFeedAction::FLatentPlayMRCaptureFeedAction(UAsyncTask_OpenMRCaptureFeedBase* InOwner)
	: Owner(InOwner)
{
	UMediaPlayer* MediaPlayer = InOwner->MediaPlayer;

	if (MediaPlayer)
	{
		OnMediaEventBinding = MediaPlayer->OnMediaEvent().AddRaw(this, &FLatentPlayMRCaptureFeedAction::HandleMediaPlayerMediaEvent);
		MediaPlayer->Play();
	}
}

FLatentPlayMRCaptureFeedAction::~FLatentPlayMRCaptureFeedAction()
{
	UMediaPlayer* MediaPlayer = Owner->MediaPlayer;
	if (MediaPlayer)
	{
		MediaPlayer->OnMediaEvent().Remove(OnMediaEventBinding);
	}
}

bool FLatentPlayMRCaptureFeedAction::IsTickable() const
{
	return Owner && Owner->MediaPlayer;
}

void FLatentPlayMRCaptureFeedAction::Tick(float DeltaTime)
{
	bool bFinished = (Owner == nullptr);
	if (!bFinished)
	{
		UMediaPlayer* MediaPlayer = Owner->MediaPlayer;
		bFinished = (MediaPlayer == nullptr);

		if (!bFinished && (MediaPlayer->HasError() ))//|| !MediaPlayer->IsPlaying()/*|| IsStopped(), etc.*/))
		{
			FMRCaptureDeviceIndex FailedFeedRef(MediaPlayer);
			Owner->OnFail.Broadcast(FailedFeedRef);

			bFinished = true;
		}
		else if (!bFinished && MediaPlayer->IsPlaying())
		{
			bFinished = true;
			for (UObject* BoundObj : Owner->OnFail.GetAllObjects())
			{
				if (IsValid(BoundObj))
				{
					bFinished = false;
					break;
				}
			}
		}
	}
	
	if (bFinished)
	{
		Destroy();
	}
}

TStatId FLatentPlayMRCaptureFeedAction::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FLatentPlayMRCaptureFeedAction, STATGROUP_ThreadPoolAsyncTasks);
}

void FLatentPlayMRCaptureFeedAction::HandleMediaPlayerMediaEvent(EMediaEvent Event)
{
	switch (Event)
	{
	case EMediaEvent::MediaOpened:
		{
			if (Owner && Owner->MediaPlayer)
			{
				// on Win7, the WMF backend has to tear down and reopen when selecting the desired track/format
				// so here we ensure we kick the MediaPlayer back to playing (in case it is set to not play-on-open)
				Owner->MediaPlayer->Play();
			}
		}
		break;

	case EMediaEvent::MediaOpenFailed:
	case EMediaEvent::PlaybackEndReached:
		{
			if (Owner)
			{
				FMRCaptureDeviceIndex FailedFeedRef(Owner->MediaPlayer);
				Owner->OnFail.Broadcast(FailedFeedRef);
			}
		}
	case EMediaEvent::MediaClosed:
		Destroy();
		break;

	default:
		break;
	}
}

void FLatentPlayMRCaptureFeedAction::Destroy()
{
	for (auto& Action : ActiveAsyncActions)
	{
		if (Action.Value.Get() == this)
		{
			if (Owner)
			{
				if (UMediaPlayer* MediaPlayer = Owner->MediaPlayer)
				{
					MediaPlayer->OnMediaEvent().Remove(OnMediaEventBinding);
				}
				Owner->CleanUp();
			}
			ActiveAsyncActions.Remove(Action.Key);
			break;
		}
	}
}

/* UAsyncTask_OpenMRCaptureFeedBase
 *****************************************************************************/

UAsyncTask_OpenMRCaptureFeedBase::UAsyncTask_OpenMRCaptureFeedBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bCachedPlayOnOpenVal(true)
{
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		AddToRoot();
	}
}

void UAsyncTask_OpenMRCaptureFeedBase::Open(UMediaPlayer* Target, const FString& DeviceURL)
{
	MediaPlayer = Target;
	// make sure nothing else is operating on this MediaPlayer
	FLatentPlayMRCaptureFeedAction::FreeAction(Target);

	if (MediaPlayer)
	{
		MediaPlayer->OnMediaOpened.AddUniqueDynamic(this, &UAsyncTask_OpenMRCaptureFeedBase::OnVideoFeedOpened);
		MediaPlayer->OnMediaOpenFailed.AddUniqueDynamic(this, &UAsyncTask_OpenMRCaptureFeedBase::OnVideoFeedOpenFailure);

		bCachedPlayOnOpenVal = MediaPlayer->PlayOnOpen;
		MediaPlayer->PlayOnOpen = false;

		bool bHasError = MediaPlayer->HasError();

		MediaPlayer->Close();
		MediaPlayer->OpenUrl(DeviceURL);
	}
}

void UAsyncTask_OpenMRCaptureFeedBase::OnVideoFeedOpened(FString DeviceUrl)
{
	FMRCaptureDeviceIndex OpenedFeedRef(MediaPlayer);
	if (MediaPlayer)
	{
		LatentPlayer = FLatentPlayMRCaptureFeedAction::Create(this);
		OnSuccess.Broadcast(OpenedFeedRef);

		// cannot remove, as we're likely iterating over this list
		//MediaPlayer->OnMediaOpened.RemoveDynamic(this, &UAsyncTask_OpenMRCaptureFeedBase::OnVideoFeedOpened);
	}
	else
	{
		OnFail.Broadcast(OpenedFeedRef);
	}
}

void UAsyncTask_OpenMRCaptureFeedBase::OnVideoFeedOpenFailure(FString DeviceUrl)
{
	CleanUp();

	FMRCaptureDeviceIndex FailedFeedRef;
	FailedFeedRef.DeviceURL = DeviceUrl;
	FailedFeedRef.StreamIndex = INDEX_NONE;
	FailedFeedRef.FormatIndex = INDEX_NONE;

	OnFail.Broadcast(FailedFeedRef);
}

bool UAsyncTask_OpenMRCaptureFeedBase::SetTrackFormat(const int32 StreamIndex, const int32 FormatIndex)
{
	bool bSuccess = false;
	if (MediaPlayer && !MediaPlayer->GetUrl().IsEmpty())
	{
		if (StreamIndex >= 0 && StreamIndex < MediaPlayer->GetNumTracks(EMediaPlayerTrack::Video))
		{
			const bool bSelected = MediaPlayer->GetSelectedTrack(EMediaPlayerTrack::Video) || MediaPlayer->SelectTrack(EMediaPlayerTrack::Video, StreamIndex);
			if (bSelected)
			{
				if (FormatIndex >= 0 && FormatIndex < MediaPlayer->GetNumTrackFormats(EMediaPlayerTrack::Video, StreamIndex))
				{
					bSuccess = (MediaPlayer->GetTrackFormat(EMediaPlayerTrack::Video, StreamIndex) == FormatIndex) || 
						MediaPlayer->SetTrackFormat(EMediaPlayerTrack::Video, StreamIndex, FormatIndex);
				}
			}
		}
	}
	return bSuccess;
}

void UAsyncTask_OpenMRCaptureFeedBase::CleanUp()
{
	if (LatentPlayer.IsValid() && FLatentPlayMRCaptureFeedAction::FindActiveAction(MediaPlayer) == LatentPlayer.Pin())
	{
		FLatentPlayMRCaptureFeedAction::FreeAction(MediaPlayer);
	}
	LatentPlayer.Reset();

	if (MediaPlayer)
	{
		MediaPlayer->OnMediaOpenFailed.RemoveDynamic(this, &UAsyncTask_OpenMRCaptureFeedBase::OnVideoFeedOpenFailure);
		MediaPlayer->OnMediaOpened.RemoveDynamic(this, &UAsyncTask_OpenMRCaptureFeedBase::OnVideoFeedOpened);

		MediaPlayer->PlayOnOpen = bCachedPlayOnOpenVal;
		MediaPlayer = nullptr;
	}
	RemoveFromRoot();
	SetReadyToDestroy();
}

/* UAsyncTask_OpenMRCaptureFeedBase
 *****************************************************************************/

UAsyncTask_OpenMRCaptureDevice* UAsyncTask_OpenMRCaptureDevice::OpenMRCaptureDevice(const FMediaCaptureDevice& DeviceId, UMediaPlayer* Target)
{
	UAsyncTask_OpenMRCaptureDevice* OpenTask = NewObject<UAsyncTask_OpenMRCaptureDevice>();
	OpenTask->Open(Target, DeviceId.Url);
	return OpenTask;
}

UAsyncTask_OpenMRCaptureDevice* UAsyncTask_OpenMRCaptureDevice::OpenMRCaptureDevice(const FMediaCaptureDeviceInfo& DeviceId, UMediaPlayer* Target, FMRCaptureFeedDelegate::FDelegate OpenedCallback)
{
	UAsyncTask_OpenMRCaptureDevice* OpenTask = NewObject<UAsyncTask_OpenMRCaptureDevice>();
	OpenTask->OnSuccess.Add(OpenedCallback);

	OpenTask->Open(Target, DeviceId.Url);
	return OpenTask;
}

UAsyncTask_OpenMRCaptureDevice::UAsyncTask_OpenMRCaptureDevice(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}

void UAsyncTask_OpenMRCaptureDevice::OnVideoFeedOpened(FString DeviceUrl)
{
	TArray<FMRCaptureDeviceIndex> SortedFormats = UMRCaptureDeviceLibrary::SortAvailableFormats(MediaPlayer);
	if (SortedFormats.Num() > 0 && ensure(DeviceUrl == SortedFormats[0].DeviceURL))
	{
		const FMRCaptureDeviceIndex& BestFormat = SortedFormats[0];
		SetTrackFormat(BestFormat.StreamIndex, BestFormat.FormatIndex);
	}

	Super::OnVideoFeedOpened(DeviceUrl);
}

/* UAsyncTask_OpenMRCaptureFeed
 *****************************************************************************/

UAsyncTask_OpenMRCaptureFeed* UAsyncTask_OpenMRCaptureFeed::OpenMRCaptureFeed(const FMRCaptureDeviceIndex& FeedRef, UMediaPlayer* Target)
{
	UAsyncTask_OpenMRCaptureFeed* OpenTask = NewObject<UAsyncTask_OpenMRCaptureFeed>();
	OpenTask->DesiredFeedRef = FeedRef;

	OpenTask->Open(Target, FeedRef.DeviceURL);
	return OpenTask;
}

UAsyncTask_OpenMRCaptureFeed* UAsyncTask_OpenMRCaptureFeed::OpenMRCaptureFeed(const FMRCaptureDeviceIndex& FeedRef, UMediaPlayer* Target, FMRCaptureFeedDelegate::FDelegate OpenedCallback)
{
	UAsyncTask_OpenMRCaptureFeed* OpenTask = NewObject<UAsyncTask_OpenMRCaptureFeed>();
	OpenTask->DesiredFeedRef = FeedRef;
	OpenTask->OnSuccess.Add(OpenedCallback);

	OpenTask->Open(Target, FeedRef.DeviceURL);
	return OpenTask;
}

UAsyncTask_OpenMRCaptureFeed::UAsyncTask_OpenMRCaptureFeed(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{}

void UAsyncTask_OpenMRCaptureFeed::OnVideoFeedOpened(FString DeviceUrl)
{
	if (ensure(DeviceUrl == DesiredFeedRef.DeviceURL))
	{
		SetTrackFormat(DesiredFeedRef.StreamIndex, DesiredFeedRef.FormatIndex);
	}
	Super::OnVideoFeedOpened(DeviceUrl);
}

#undef LOCTEXT_NAMESPACE
