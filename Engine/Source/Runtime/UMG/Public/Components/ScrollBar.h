// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ScrollBar.generated.h"

/** */
UCLASS(meta=( Category="Primitive" ), ClassGroup=UserInterface, meta=( IsAdvanced = "True" ))
class UMG_API UScrollBar : public UPanelWidget
{
	GENERATED_UCLASS_BODY()

	/** Style of the scrollbar */
	UPROPERTY(EditDefaultsOnly, Category="Style", meta=( DisplayThumbnail = "true" ))
	USlateWidgetStyleAsset* Style;

	/**  */
	UPROPERTY(EditDefaultsOnly, Category="Behavior")
	bool bAlwaysShowScrollbar;

	/**  */
	UPROPERTY(EditDefaultsOnly, Category="Behavior")
	TEnumAsByte<EOrientation> Orientation;

	/** The thickness of the scrollbar thumb */
	UPROPERTY(EditDefaultsOnly, Category="Behavior")
	FVector2D Thickness;

	/**
	* Set the offset and size of the track's thumb.
	* Note that the maximum offset is 1.0-ThumbSizeFraction.
	* If the user can view 1/3 of the items in a single page, the maximum offset will be ~0.667f
	*
	* @param InOffsetFraction     Offset of the thumbnail from the top as a fraction of the total available scroll space.
	* @param InThumbSizeFraction  Size of thumbnail as a fraction of the total available scroll space.
	*/
	UFUNCTION(BlueprintCallable, Category="Scrolling")
	void SetState(float InOffsetFraction, float InThumbSizeFraction);

	///** @return true if scrolling is possible; false if the view is big enough to fit all the content */
	//bool IsNeeded() const;

	///** @return normalized distance from top */
	//float DistanceFromTop() const;

	///** @return normalized distance from bottom */
	//float DistanceFromBottom() const;

	///** @return the scrollbar's visibility as a product of internal rules and user-specified visibility */
	//EVisibility ShouldBeVisible() const;

	///** @return True if the user is scrolling by dragging the scroll bar thumb. */
	//bool IsScrolling() const;


	// UWidget interface
	virtual void SyncronizeProperties() override;
	// End of UWidget interface

#if WITH_EDITOR
	// UWidget interface
	virtual const FSlateBrush* GetEditorIcon() override;
	// End UWidget interface
#endif

protected:

	TSharedPtr<class SScrollBar> MyScrollBar;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface
};
