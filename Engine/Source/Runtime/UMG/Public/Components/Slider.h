// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Slider.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMouseCaptureBeginEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMouseCaptureEndEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFloatValueChangedEvent, float, Value);

/** Slider widget */
UCLASS(meta=( Category="Common" ), ClassGroup=UserInterface)
class UMG_API USlider : public UWidget
{
	GENERATED_UCLASS_BODY()
	
	///** The style used to draw the slider. */
	//SLATE_STYLE_ARGUMENT(FSliderStyle, Style)

	/** The volume value to display. */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	float Value;

	/** The slider's orientation. */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	TEnumAsByte<EOrientation> Orientation;

	/** The color to draw the slider bar in. */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FLinearColor SliderBarColor;

	/** The color to draw the slider handle in. */
	UPROPERTY(EditDefaultsOnly, Category=Appearance)
	FLinearColor SliderHandleColor;

	/** Whether the slidable area should be indented to fit the handle. */
	UPROPERTY(EditDefaultsOnly, Category=Appearance, AdvancedDisplay)
	bool IndentHandle;

	/** Whether the handle is interactive or fixed. */
	UPROPERTY(EditDefaultsOnly, Category=Appearance, AdvancedDisplay)
	bool Locked;

	/** Invoked when the mouse is pressed and a capture begins. */
	UPROPERTY(BlueprintAssignable)
	FOnMouseCaptureBeginEvent OnMouseCaptureBegin;

	/** Invoked when the mouse is released and a capture ends. */
	UPROPERTY(BlueprintAssignable)
	FOnMouseCaptureEndEvent OnMouseCaptureEnd;

	/** Called when the value is changed by slider or typing. */
	UPROPERTY(BlueprintAssignable)
	FOnFloatValueChangedEvent OnValueChanged;

	/** Gets the current value of the slider. */
	UFUNCTION(BlueprintPure, Category="Behavior")
	float GetValue();

	/** Sets the current value of the slider. */
	UFUNCTION(BlueprintCallable, Category="Behavior")
	void SetValue(float InValue);
	
	// UWidget interface
	virtual void SyncronizeProperties() override;
	// End of UWidget interface

#if WITH_EDITOR
	virtual const FSlateBrush* GetEditorIcon() override;
#endif

protected:
	/** Native Slate Widget */
	TSharedPtr<SSlider> MySlider;

	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface

	void HandleOnValueChanged(float InValue);
	void HandleOnMouseCaptureBegin();
	void HandleOnMouseCaptureEnd();
};
