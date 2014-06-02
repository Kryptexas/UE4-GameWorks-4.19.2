// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ProgressBar.generated.h"

/** ProgressBar widget */
UCLASS(meta=(BlueprintSpawnableComponent), ClassGroup=UserInterface)
class UMG_API UProgressBar : public UWidget
{
	GENERATED_UCLASS_BODY()

	///** Style used for the progress bar */
	//UPROPERTY(EditDefaultsOnly, Category=Appearance)
	//FProgressBarStyle Style;

	/** Defines if this progress bar fills Left to right or right to left */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	TEnumAsByte<EProgressBarFillType::Type> BarFillType;

	/** Used to determine the fill position of the progress bar ranging 0..1 */
	UPROPERTY(EditDefaultsOnly, Category=Appearance, meta=( UIMin = "0", UIMax = "1" ))
	float Percent;
	//TODO UMG Slate supports TOptional<float> for Percent.  UMG should as well.

	/** Fill Color and Opacity */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FLinearColor FillColorAndOpacity;

	/** Border Padding around fill bar */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FVector2D BorderPadding;

	/** Gets the current value of the ProgressBar. */
	UFUNCTION(BlueprintPure, Category="Behavior")
	float GetPercent();

	/** Sets the current value of the ProgressBar. */
	UFUNCTION(BlueprintCallable, Category="Behavior")
	void SetPercent(float InPercent);

protected:
	/** Native Slate Widget */
	TSharedPtr<SProgressBar> MyProgressBar;

	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() OVERRIDE;
	// End of UWidget interface
};
