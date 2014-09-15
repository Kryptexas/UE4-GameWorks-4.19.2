// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ProgressWidgetStyle.h"

#include "ProgressBar.generated.h"

/** ProgressBar widget */
UCLASS(ClassGroup=UserInterface)
class UMG_API UProgressBar : public UWidget
{
	GENERATED_UCLASS_BODY()
	
public:

	/** The progress bar style */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style", meta=( DisplayName="Style" ))
	FProgressBarStyle WidgetStyle;

	/** Style used for the progress bar */
	UPROPERTY()
	USlateWidgetStyleAsset* Style_DEPRECATED;

	/** The brush to use as the background of the progress bar */
	UPROPERTY()
	USlateBrushAsset* BackgroundImage_DEPRECATED;
	
	/** The brush to use as the fill image */
	UPROPERTY()
	USlateBrushAsset* FillImage_DEPRECATED;
	
	/** The brush to use as the marquee image */
	UPROPERTY()
	USlateBrushAsset* MarqueeImage_DEPRECATED;

	/** Defines if this progress bar fills Left to right or right to left */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	TEnumAsByte<EProgressBarFillType::Type> BarFillType;
	
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	bool bIsMarquee;

	/** Used to determine the fill position of the progress bar ranging 0..1 */
	UPROPERTY(EditDefaultsOnly, Category=Appearance, meta=( UIMin = "0", UIMax = "1" ))
	float Percent;

	/** A bindable delegate to allow logic to drive the text of the widget */
	UPROPERTY()
	FGetFloat PercentDelegate;

	/** Fill Color and Opacity */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FLinearColor FillColorAndOpacity;

public:
	
	/** Sets the current value of the ProgressBar. */
	UFUNCTION(BlueprintCallable, Category="Behavior")
	void SetPercent(float InPercent);

	/** Sets the progress bar to show as a marquee. */
	UFUNCTION(BlueprintCallable, Category="Behavior")
	void SetIsMarquee(bool InbIsMarquee);
	
	// UWidget interface
	virtual void SynchronizeProperties() override;
	// End of UWidget interface

	// UVisual interface
	virtual void ReleaseNativeWidget() override;
	// End of UVisual interface

	// Begin UObject interface
	virtual void PostLoad() override;
	// End of UObject interface

#if WITH_EDITOR
	// UWidget interface
	virtual const FSlateBrush* GetEditorIcon() override;
	virtual const FText GetPaletteCategory() override;
	// End UWidget interface
#endif

protected:
	/** Native Slate Widget */
	TSharedPtr<SProgressBar> MyProgressBar;

	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface
};
