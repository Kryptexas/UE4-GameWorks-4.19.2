// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/TextProperty.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Misc/MediaBlueprintFunctionLibrary.h" // for FMediaCaptureDevice
#include "Tickable.h"
#include "MixedRealityCaptureDevice.generated.h"

class  UMediaPlayer;
struct FMediaCaptureDeviceInfo;

/**
 *	
 */
USTRUCT(BlueprintType)
struct FMRCaptureDeviceIndex
{
	GENERATED_USTRUCT_BODY()

public:
	FMRCaptureDeviceIndex();
	FMRCaptureDeviceIndex(UMediaPlayer* MediaPlayer);
	FMRCaptureDeviceIndex(FMediaCaptureDeviceInfo& DeviceInfo);

	UPROPERTY(BlueprintReadWrite, Category="MixedReality|CaptureDevice")
	FString DeviceURL;
	UPROPERTY(BlueprintReadWrite, Category="MixedReality|CaptureDevice")
	int32 StreamIndex;
	UPROPERTY(BlueprintReadWrite, Category="MixedReality|CaptureDevice")
	int32 FormatIndex;

	bool IsSet(UMediaPlayer* MediaPlayer) const;
	bool IsDeviceUrlValid() const;

	bool operator==(const FMRCaptureDeviceIndex& Rhs) const;
	bool operator!=(const FMRCaptureDeviceIndex& Rhs) const;
};

/**
 *	
 */
UCLASS()
class UMRCaptureDeviceLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "MixedReality|Utils")
	static void GetFeedDescription(UMediaPlayer* MediaPlayer, FText& DeviceName, FText& FormatDesc);

	UFUNCTION(BlueprintCallable, Category="MixedReality|Utils")
	static TArray<FMRCaptureDeviceIndex> SortAvailableFormats(UMediaPlayer* MediaPlayer);
};

/* UAsyncTask_OpenMRCaptureFeedBase
 *****************************************************************************/

struct FLatentPlayMRCaptureFeedAction;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMRCaptureFeedDelegate, const FMRCaptureDeviceIndex&, FeedRef);

UCLASS(Abstract)
class UAsyncTask_OpenMRCaptureFeedBase : public UBlueprintAsyncActionBase
{
	GENERATED_UCLASS_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FMRCaptureFeedDelegate OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FMRCaptureFeedDelegate OnFail;

public:
	void Open(UMediaPlayer* Target, const FString& DeviceURL);

protected:
	UFUNCTION()
	virtual void OnVideoFeedOpened(FString DeviceUrl);

	UFUNCTION()
	virtual void OnVideoFeedOpenFailure(FString DeviceUrl);

	bool SetTrackFormat(const int32 StreamIndex, const int32 FormatIndex);
	void CleanUp();

	UPROPERTY()
	UMediaPlayer* MediaPlayer;
	
private:
	friend struct FLatentPlayMRCaptureFeedAction;
	bool bCachedPlayOnOpenVal;

	TWeakPtr<FLatentPlayMRCaptureFeedAction> LatentPlayer;
};

/* UAsyncTask_OpenMRCaptureDevice
 *****************************************************************************/

UCLASS()
class UAsyncTask_OpenMRCaptureDevice : public UAsyncTask_OpenMRCaptureFeedBase
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true"), Category="MixedReality|Utils")
	static UAsyncTask_OpenMRCaptureDevice* OpenMRCaptureDevice(const FMediaCaptureDevice& DeviceId, UMediaPlayer* Target);
	
	static UAsyncTask_OpenMRCaptureDevice* OpenMRCaptureDevice(const FMediaCaptureDeviceInfo& DeviceId, UMediaPlayer* Target, FMRCaptureFeedDelegate::FDelegate OpenedCallback);

private:
	UFUNCTION()
	virtual void OnVideoFeedOpened(FString DeviceUrl) override;
};

/* UAsyncTask_OpenMRCaptureFeed
 *****************************************************************************/

UCLASS()
class UAsyncTask_OpenMRCaptureFeed : public UAsyncTask_OpenMRCaptureFeedBase
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true"), Category="MixedReality|Utils")
	static UAsyncTask_OpenMRCaptureFeed* OpenMRCaptureFeed(const FMRCaptureDeviceIndex& FeedRef, UMediaPlayer* Target);

	static UAsyncTask_OpenMRCaptureFeed* OpenMRCaptureFeed(const FMRCaptureDeviceIndex& FeedRef, UMediaPlayer* Target, FMRCaptureFeedDelegate::FDelegate OpenedCallback);

private:
	UFUNCTION()
	virtual void OnVideoFeedOpened(FString DeviceUrl) override;

	FMRCaptureDeviceIndex DesiredFeedRef;
};
