// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"
#include "IMediaOverlaySink.h"

#include "MediaOverlays.generated.h"


USTRUCT(BlueprintType)
struct FMediaPlayerOverlay
{
	GENERATED_BODY()

	/** Whether the text position is set. */
	UPROPERTY()
	bool HasPosition;

	/** The text position. */
	UPROPERTY()
	FVector2D Position;

	/** The overlay text. */
	UPROPERTY()
	FText Text;
};


/**
* Implements an asset that collects overlay texts from UMediaPlayer assets.
*/
UCLASS(BlueprintType, hidecategories=(Object))
class MEDIAASSETS_API UMediaOverlays
	: public UObject
	, public IMediaOverlaySink
{
	/** Text overlay entry. */
	struct FOverlay
	{
		FTimespan Duration;
		TOptional<FVector2D> Position;
		FText Text;
		FTimespan Time;
		EMediaOverlayType Type;
	};

	GENERATED_BODY()

public:

	/**
	 * Get the current caption text overlays for the specified time, if any.
	 *
	 * @param OutCaptions Will contain the caption text overlays.
	 * @param Time The for which to get the overlays.
	 * @see GetSubtitles, GetTexts
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	void GetCaptions(TArray<FMediaPlayerOverlay>& OutCaptions, FTimespan Time) const
	{
		GetItems(EMediaOverlayType::Caption, OutCaptions, Time);
	}

	/**
	 * Get the current subtitle text overlays for the specified time, if any.
	 *
	 * @param OutSubtitles Will contain the caption text overlays.
	 * @param Time The for which to get the overlays.
	 * @see GetCaptions, GetTexts
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	void GetSubtitles(TArray<FMediaPlayerOverlay>& OutSubtitles, FTimespan Time) const
	{
		GetItems(EMediaOverlayType::Subtitle, OutSubtitles, Time);
	}

	/**
	 * Get the current generic text overlays for the specified time, if any.
	 *
	 * @param OutTexts Will contain the text overlays.
	 * @param Time The for which to get the overlays.
	 * @see GetCaptions, GetSubtitles
	 */
	UFUNCTION(BlueprintCallable, Category="Media|MediaPlayer")
	void GetTexts(TArray<FMediaPlayerOverlay>& OutTexts, FTimespan Time) const
	{
		GetItems(EMediaOverlayType::Text, OutTexts, Time);
	}

public:

	/**
	* Get the text overlay items of the given type at the specified time.
	*
	* @param Type The type of overlays to get, i.e. captions, subtitles, etc.
	* @param Time The for which to get the overlays.
	* @param OutItems Will contain the text overlay items.
	*/
	void GetItems(EMediaOverlayType Type, TArray<FMediaPlayerOverlay>& OutItems, FTimespan Time) const;

public:

	/**
	 * Get the event delegate that is invoked when this asset is being destroyed.
	 *
	 * @return The delegate.
	 */
	DECLARE_EVENT_OneParam(UMediaOverlays, FOnBeginDestroy, UMediaOverlays& /*DestroyedOverlays*/)
	FOnBeginDestroy& OnBeginDestroy()
	{
		return BeginDestroyEvent;
	}

public:

	//~ UObject interface.

	virtual void BeginDestroy() override;

public:

	//~ IMediaOverlaySink interface

	virtual bool InitializeOverlaySink() override;
	virtual void AddOverlaySinkText(const FText& Text, EMediaOverlayType Type, FTimespan Time, FTimespan Duration, TOptional<FVector2D> Position) override;
	virtual void ClearOverlaySinkText() override;
	virtual void ShutdownOverlaySink() override;

private:

	/** An event delegate that is invoked when this asset is being destroyed. */
	FOnBeginDestroy BeginDestroyEvent;

	/** Queued text overlays. */
	TArray<FOverlay> Overlays;
};
